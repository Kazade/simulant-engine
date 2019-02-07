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
#include "../../nodes/camera.h"

#include "render_queue.h"
#include "../../partitioner.h"

namespace smlt {
namespace batcher {


RenderQueue::RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory, CameraPtr camera):
    stage_(stage),
    render_group_factory_(render_group_factory),
    camera_(camera) {

}

void RenderQueue::insert_renderable(std::shared_ptr<Renderable> renderable) {
    /*
     * Adds a renderable to the correct render groups. This goes through the
     * material passes on the renderable, calculates the render group for each one
     * and then adds the renderable to that pass's render queue
     */

    auto material_id = renderable->material_id();
    assert(material_id);

    auto material = stage_->assets->material(material_id);
    assert(material);

    auto pos = renderable->transformed_aabb().centre();
    auto plane = camera_->frustum().plane(FRUSTUM_PLANE_NEAR);
    auto renderable_dist_to_camera = plane.distance_to(pos);

    material->each([&](uint32_t i, MaterialPass* material_pass) {

        bool is_blended = material_pass->is_blended();

        // If we're not blending, we don't bother sorting based on distance
        // as grouping by texture is more important
        // FIXME: Experiment with sorting non-blended in front-to-back order
        // to see if it improves performance
        float distance_to_camera = (!is_blended) ? 0.0f : (
            renderable_dist_to_camera
        );

        RenderGroup group = render_group_factory_->new_render_group(
            renderable.get(), material_pass,
            renderable->render_priority(), is_blended, distance_to_camera
        );

        assert(i < MAX_MATERIAL_PASSES);
        assert(i < material->pass_count());

        if(batches_.size() <= i) {
            batches_.push_back(BatchMap());
        }

        auto& batch = batches_[i];

        auto it = batch.find(group);
        if(it == batch.end()) {
            it = batch.insert(std::make_pair(group, std::make_shared<Batch>())).first;
        }

        it->second->add_renderable(renderable);
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
        MaterialPass* material_pass = nullptr;

        const RenderGroup* last_group = nullptr;

        for(auto& p: batches) {
            const RenderGroup* current_group = &p.first;

            // Track whether we've called change_render_group yet
            bool render_group_changed_ = false;

            p.second->each([&](uint32_t i, Renderable* renderable) {
                if(!renderable->is_visible_in_frame(frame_id)) {
                    return;
                }

                if(!renderable->index_element_count()) {
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
                    pass_iteration_type = material_pass->iteration_type();

                    visitor->change_material_pass(last_pass, material_pass);
                }

                uint32_t iterations = 1;

                // Get any lights which are visible and affecting the renderable this frame
                std::vector<LightPtr> lights = renderable->lights_affecting_this_frame();

                if(pass_iteration_type == ITERATION_TYPE_N) {
                    iterations = material_pass->max_iterations();
                } else if(pass_iteration_type == ITERATION_TYPE_ONCE_PER_LIGHT) {
                    iterations = lights.size();
                }

                for(Iteration i = 0; i < iterations; ++i) {
                    LightPtr next = nullptr;

                    // Pass down the light if necessary, otherwise just pass nullptr
                    if(!lights.empty()) {
                        next = lights[i];
                    } else {
                        next = nullptr;
                    }

                    if(pass_iteration_type == ITERATION_TYPE_ONCE_PER_LIGHT) {
                        visitor->apply_lights(&next, 1);
                    } else if(pass_iteration_type == ITERATION_TYPE_N || pass_iteration_type == ITERATION_TYPE_ONCE) {
                        visitor->apply_lights(&lights[0], (uint8_t) lights.size());
                    }
                    visitor->visit(renderable, material_pass, i);
                }

                last_group = current_group;
            });
        }
        ++pass;
    }

    visitor->end_traversal(*this, stage_);
}

void Batch::add_renderable(RenderablePtr renderable) {
    assert(renderable);

    write_lock<shared_mutex> lock(batch_lock_);

    renderable->join_batch(this);
    renderables_.push_back(renderable);
}

void Batch::remove_renderable(RenderablePtr renderable) {
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
        func(i++, renderable.get());
    }
}

}
}
