
#include "../../stage.h"
#include "../../material.h"
#include "../../actor.h"
#include "../../particles.h"

#include "render_queue.h"
#include "../../partitioner.h"

namespace kglt {
namespace batcher {

RenderQueue::RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory):
    stage_(stage),
    render_group_factory_(render_group_factory) {

    stage->signal_actor_created().connect([=](ActorID actor_id) {
        auto actor = stage->actor(actor_id);
        actor->each([=](uint32_t i, SubActor* subactor) {
            insert_renderable(subactor);
        });
    });

    stage->signal_actor_destroyed().connect([=](ActorID actor_id) {
        auto actor = stage->actor(actor_id);
        actor->each([=](uint32_t i, SubActor* subactor) {
            remove_renderable(subactor);
        });
    });

    stage->signal_actor_changed().connect([=](ActorID actor_id, ActorChangeEvent event) {
        auto actor = stage->actor(actor_id);
        if(event.type == ACTOR_CHANGE_TYPE_SUBACTOR_MATERIAL_CHANGED) {
            actor->each([=](uint32_t i, SubActor* subactor) {
                remove_renderable(subactor);
                insert_renderable(subactor);
            });
        }
    });

    stage->signal_particle_system_created().connect([=](ParticleSystemID ps_id) {
        auto ps = stage->particle_system(ps_id);
        insert_renderable(ps.get());
    });

    stage->signal_particle_system_destroyed().connect([=](ParticleSystemID ps_id) {
        auto ps = stage->particle_system(ps_id);
        remove_renderable(ps.get());
    });
}

void RenderQueue::insert_renderable(Renderable* renderable) {
    /*
     * Adds a renderable to the correct render groups. This goes through the
     * material passes on the renderable, calculates the render group for each one
     * and then adds the renderable to that pass's render queue
     */

    auto material_id = renderable->material_id();
    auto material = stage_->assets->material(material_id);

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
    for(auto pass = batches_.begin(); pass != batches_.end();) {
        for(auto group = pass->begin(); group != pass->end();) {
            if(!group->second.renderable_count()) {
                pass->erase(group++);
            } else {
                ++group;
            }
        }

        if(pass->empty()) {
            pass = batches_.erase(pass);
        } else {
            ++pass;
        }
    }
}

void RenderQueue::remove_renderable(Renderable* renderable) {
    /*
     * The Renderable is a BatchMember had maintains a list of
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

                material = stage_->assets->material(renderable->material_id());
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
                        light = lights[i].get();
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
    renderable->join_batch(this);
    renderables_.push_back(renderable);
}

void Batch::remove_renderable(Renderable *renderable) {
    auto it = std::find(renderables_.begin(), renderables_.end(), renderable);
    if(it != renderables_.end()) {
        renderables_.erase(it);
    }
    renderable->leave_batch(this);
}

}
}
