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
#include "../../nodes/particle_system.h"
#include "../../nodes/geom.h"
#include "../../nodes/geoms/geom_culler.h"

#include "render_queue.h"
#include "../../partitioner.h"

namespace smlt {
namespace batcher {


RenderQueue::RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory):
    stage_(stage),
    render_group_factory_(render_group_factory) {

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

        if(!batches_[i].count(group)) {
            batches_[i].insert(std::make_pair(group, std::make_shared<Batch>()));
        }

        batches_[i][group]->add_renderable(renderable);
    });
}

void RenderQueue::clean_empty_batches() {    
    std::lock_guard<std::mutex> lock(queue_lock_);

    for(auto& pass: batches_) {
        auto group = pass.begin();
        while(group != pass.end()) {
            auto& batch = group->second;
            if(!batch->renderable_count()) {
                // Remove the group in a loop-safe way
                group = pass.erase(group);
            } else {
                ++group;
            }
        }
    }

    for(auto it = batches_.begin(); it != batches_.end();) {
        if(it->empty()) {
            it = batches_.erase(it);
        } else {
            ++it;
        }
    }
}

void RenderQueue::clear() {
    std::lock_guard<std::mutex> lock(queue_lock_);
    batches_.clear();
}

void RenderQueue::traverse(RenderQueueVisitor* visitor, uint64_t frame_id) const {
    std::lock_guard<std::mutex> lock(queue_lock_);

    Pass pass = 0;

    visitor->start_traversal(*this, frame_id, stage_);

    for(auto& batches: batches_) {
        IterationType pass_iteration_type;
        MaterialID material_id;
        MaterialPass::ptr material_pass;

        const RenderGroup* last_group = nullptr;

        for(auto& p: batches) {
            const RenderGroup* current_group = &p.first;

            // Track whether we've called change_render_group yet
            bool render_group_changed_ = false;

            p.second->each([&](uint32_t i, Renderable* renderable) {
                if(!renderable->is_visible_in_frame(frame_id)) {
                    return;
                }

                /* We do this here so that we don't change render group unless something in the
                 * new group is visible */
                if(!render_group_changed_) {
                    visitor->change_render_group(last_group, current_group);
                    render_group_changed_ = true;
                }


                /* As the pass number is constant for the entire batch, a material_pass
                 * will only change if and when a material changes
                 */
                auto& this_mat_id = renderable->material_id();
                if(this_mat_id != material_id) {
                    auto last_pass = material_pass;

                    material_id = this_mat_id;
                    material_pass = stage_->assets->material(material_id)->pass(pass);
                    pass_iteration_type = material_pass->iteration();

                    visitor->change_material_pass(last_pass.get(), material_pass.get());
                }

                uint32_t iterations = 1;

                // Get any lights which are visible and affecting the renderable this frame
                std::vector<LightPtr> lights = renderable->lights_affecting_this_frame();

                if(pass_iteration_type == ITERATE_N) {
                    iterations = material_pass->max_iterations();
                } else if(pass_iteration_type == ITERATE_ONCE_PER_LIGHT) {
                    iterations = lights.size();
                }

                Light* light = nullptr;
                for(Iteration i = 0; i < iterations; ++i) {
                    Light* next = nullptr;

                    // Pass down the light if necessary, otherwise just pass nullptr
                    if(!lights.empty()) {
                        next = lights[i];
                    } else {
                        next = nullptr;
                    }

                    if(pass_iteration_type == ITERATE_ONCE_PER_LIGHT && (i== 0 || light != next)) {
                        visitor->change_light(light, next);
                    } else if(pass_iteration_type == ITERATE_N || pass_iteration_type == ITERATE_ONCE) {
                        visitor->apply_lights(&lights[0], (uint8_t) lights.size());
                    }

                    light = next;
                    visitor->visit(renderable, material_pass.get(), i);
                }

                last_group = current_group;
            });
        }
        ++pass;
    }

    visitor->end_traversal(*this, stage_);
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
