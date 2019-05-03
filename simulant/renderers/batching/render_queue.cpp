//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
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

    if(!renderable->is_visible()) {
        return;
    }

    if(!renderable->index_element_count()) {
        return;
    }

    auto material_id = renderable->material_id();
    assert(material_id);

    auto material = stage_->assets->material(material_id);
    assert(material);

    auto pos = renderable->transformed_aabb().centre();
    auto plane = camera_->frustum().plane(FRUSTUM_PLANE_NEAR);
    auto renderable_dist_to_camera = plane.distance_to(pos);
    auto priority = renderable->render_priority();

    auto pass_count = material->pass_count();
    for(auto i = 0u; i < pass_count; ++i) {
        MaterialPass* material_pass = material->pass(i);

        bool is_blended = material_pass->is_blending_enabled();

        // If we're not blending, we don't bother sorting based on distance
        // as grouping by texture is more important
        // FIXME: Experiment with sorting non-blended in front-to-back order
        // to see if it improves performance
        float distance_to_camera = (!is_blended) ? 0.0f : (
            renderable_dist_to_camera
        );

        RenderGroup group = render_group_factory_->new_render_group(
            renderable.get(), material_pass,
            i, is_blended, distance_to_camera
        );

        auto& priority_queue = priority_queues_[priority];
        priority_queue.insert(std::make_pair(group, renderable));
    }
}


void RenderQueue::clear() {
    std::lock_guard<std::mutex> lock(queue_lock_);
    for(auto& queue: priority_queues_) {
        queue.clear();
    }
}

void RenderQueue::traverse(RenderQueueVisitor* visitor, uint64_t frame_id) const {
    std::lock_guard<std::mutex> lock(queue_lock_);

    Pass pass = 0;

    visitor->start_traversal(*this, frame_id, stage_);

    for(auto& queue: priority_queues_) {
        IterationType pass_iteration_type = ITERATION_TYPE_ONCE;
        MaterialID material_id;
        MaterialPass* material_pass = nullptr;

        const RenderGroup* last_group = nullptr;

        for(auto& p: queue) {
            const RenderGroup* current_group = &p.first;
            Renderable* renderable = p.second.get();

            if(!renderable->index_element_count()) {
                return;
            }

            /* We do this here so that we don't change render group unless something in the
             * new group is visible */
            if(!last_group || *current_group != *last_group) {
                visitor->change_render_group(last_group, current_group);
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

        }
        ++pass;
    }

    visitor->end_traversal(*this, stage_);
}

uint32_t RenderQueue::group_count(Pass pass_number) const {
    uint32_t i = 0;
    auto& queue = priority_queues_.at(pass_number);
    for(auto it = queue.begin(); it != queue.end(); it = queue.upper_bound(it->first)) {
        ++i;
    }

    return i;
}

}
}
