//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//


#include "../../stage.h"
#include "../../material.h"
#include "../../nodes/actor.h"
#include "../../nodes/particles.h"

#include "render_queue.h"
#include "../../partitioner.h"

namespace smlt {
namespace batcher {

void reinsert(ActorID actor_id, RenderQueue* queue) {
    auto actor = actor_id.fetch();
    actor->each([queue](uint32_t i, SubActor* subactor) {
        queue->remove_renderable(subactor);
        queue->insert_renderable(subactor);
    });
}

RenderQueue::RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory):
    stage_(stage),
    render_group_factory_(render_group_factory) {

    stage->signal_actor_created().connect([=](ActorID actor_id) {
        auto actor = stage->actor(actor_id);
        actor->each([=](uint32_t i, SubActor* subactor) {
            insert_renderable(subactor);
        });

        // If the actor's render priority changes, we need to remove the renderables and re-add them
        actor->signal_render_priority_changed().connect([this, actor_id](RenderPriority old, RenderPriority newp) {
            reinsert(actor_id, this);
        });


        // If a material changes on an actor's subactor, reinsert
        actor->signal_subactor_material_changed().connect([this](ActorID, SubActor* subactor, MaterialID, MaterialID) {
            remove_renderable(subactor);
            insert_renderable(subactor);
        });

        // If a new subactor is added to the actor, add it to the render queue
        actor->signal_subactor_created().connect([this](ActorID actor_id, SubActor* subactor) {
            insert_renderable(subactor);
        });

        // If a subactor is destroyed, remove it from the render queue
        actor->signal_subactor_destroyed().connect([this](ActorID actor_id, SubActor* subactor) {
            remove_renderable(subactor);
        });
    });

    stage->signal_actor_destroyed().connect([=](ActorID actor_id) {
        auto actor = stage->actor(actor_id);
        actor->each([=](uint32_t i, SubActor* subactor) {
            remove_renderable(subactor);
        });
    });

    stage->signal_particle_system_created().connect([=](ParticleSystemID ps_id) {
        auto ps = stage->particle_system(ps_id);
        insert_renderable(ps);
        ps->signal_render_priority_changed().connect([this, ps_id](RenderPriority old, RenderPriority newp) {
            auto ps = ps_id.fetch();
            remove_renderable(ps);
            insert_renderable(ps);
        });
    });

    stage->signal_particle_system_destroyed().connect([=](ParticleSystemID ps_id) {
        auto ps = stage->particle_system(ps_id);
        remove_renderable(ps);
    });
}

void RenderQueue::insert_renderable(Renderable* renderable) {
    /*
     * Adds a renderable to the correct render groups. This goes through the
     * material passes on the renderable, calculates the render group for each one
     * and then adds the renderable to that pass's render queue
     */

    auto material_id = renderable->material_id();
    assert(material_id);

    auto material = stage_->assets->material(material_id);
    assert(material);

    material->each([&](uint32_t i, MaterialPass* material_pass) {
        RenderGroup group = render_group_factory_->new_render_group(
            renderable, material_pass
        );

        assert(i < MAX_MATERIAL_PASSES);
        assert(i < material->pass_count());

        if(batches_.size() <= i) {
            batches_.push_back(BatchMap());
        }

        batches_[i][group].add_renderable(renderable);
    });
}

void RenderQueue::clean_empty_batches() {
    for(auto& pass: batches_) {
        auto group = pass.begin();
        while(group != pass.end()) {
            auto& batch = group->second;
            if(!batch.renderable_count()) {
                group = pass.erase(group);
            } else {
                ++group;
            }
        }
    }

    batches_.erase(
        std::remove_if(batches_.begin(), batches_.end(), [](const BatchMap& pass) -> bool { return pass.empty(); }),
        batches_.end()
    );
}

void RenderQueue::remove_renderable(Renderable* renderable) {
    /*
     * The Renderable is a BatchMember that maintains a list of
     * batches that it is a member of. This way removing the renderable
     * is just a case of iterating its batches and removing it. If the
     * renderable was the last in the batch we need to remove the batch.
     */

    bool empty_batches_created = false;
    for(auto batch: renderable->batches()) {
        batch->remove_renderable(renderable);
        if(!batch->renderable_count()) {
            empty_batches_created = true;
        }
    }

    if(empty_batches_created) {
        clean_empty_batches();
    }
}

void RenderQueue::traverse(TraverseCallback callback, uint64_t frame_id) const {
    Pass pass = 0;
    for(auto& batches: batches_) {
        IterationType pass_iteration_type;

        for(auto& p: batches) {
            const RenderGroup* current_group = &p.first;
            MaterialPtr material;
            MaterialPass::ptr material_pass;

            bool render_group_changed = true;
            p.second.each([&](uint32_t i, Renderable* renderable) {
                if(!renderable->is_visible_in_frame(frame_id)) {
                    return;
                }

                auto& material_id = renderable->material_id();
                assert(material_id);

                material = stage_->assets->material(material_id);
                assert(material);

                material_pass = material->pass(pass);
                pass_iteration_type = material_pass->iteration();

                uint32_t iterations = 1;


                std::vector<LightPtr> lights;

                if(pass_iteration_type == ITERATE_N) {
                    iterations = material_pass->max_iterations();
                } else if(pass_iteration_type == ITERATE_ONCE_PER_LIGHT) {
                    // Get any lights which are visible and affecting the renderable this frame
                    lights = renderable->lights_affecting_this_frame();
                    iterations = lights.size();
                }

                Light* light = nullptr;
                for(Iteration i = 0; i < iterations; ++i) {
                    // Pass down the light if necessary, otherwise just pass nullptr
                    if(!lights.empty()) {
                        light = lights[i];
                    } else {
                        light = nullptr;
                    }

                    callback(render_group_changed, current_group, renderable, material_pass.get(), light, i);
                }

                render_group_changed = false;
            });
        }
        ++pass;
    }
}

void Batch::add_renderable(Renderable* renderable) {
    assert(renderable);

    write_lock<shared_mutex> lock(batch_lock_);

    renderable->join_batch(this);
    renderables_.push_back(renderable);
}

void Batch::remove_renderable(Renderable *renderable) {
    write_lock<shared_mutex> lock(batch_lock_);

    auto it = std::find(renderables_.begin(), renderables_.end(), renderable);
    if(it != renderables_.end()) {
        renderables_.erase(it);
    }
    renderable->leave_batch(this);
}

void Batch::each(std::function<void (uint32_t, Renderable *)> func) const {
    read_lock<shared_mutex> lock(batch_lock_);

    uint32_t i = 0;
    for(auto& renderable: renderables_) {
        func(i++, renderable);
    }
}

}
}
