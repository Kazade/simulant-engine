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


RenderGroupKey generate_render_group_key(const uint8_t pass, const bool is_blended, const float distance_to_camera) {
    RenderGroupKey key;
    key.pass = pass;
    key.is_blended = is_blended;
    key.distance_to_camera = distance_to_camera;
    return key;
}

RenderQueue::RenderQueue() {

}

void RenderQueue::reset(Stage* stage, RenderGroupFactory* factory, CameraPtr camera) {
    stage_ = stage;
    render_group_factory_ = factory;
    camera_ = camera;

    clear();
}

void RenderQueue::insert_renderable(Renderable&& src_renderable) {
    /*
     * Adds a renderable to the correct render groups. This goes through the
     * material passes on the renderable, calculates the render group for each one
     * and then adds the renderable to that pass's render queue
     */

    assert(stage_);
    assert(camera_);
    assert(render_group_factory_);

    auto idx = renderables_.size();
    renderables_.push_back(src_renderable);
    auto renderable = &renderables_.back();

    if(!renderable->is_visible || !renderable->index_element_count) {
        renderables_.pop_back();
        return;
    }

    auto material = renderable->material;
    assert(material);

    auto pos = renderable->centre;
    auto plane = camera_->frustum().plane(FRUSTUM_PLANE_NEAR);
    auto renderable_dist_to_camera = plane.distance_to(pos);
    auto priority = renderable->render_priority;

    auto pass_count = material->pass_count();
    for(auto i = 0u; i < pass_count; ++i) {
        MaterialPass* pass = material->pass(i);
        RenderGroup group;

        bool is_blended = pass->is_blending_enabled();

        group.sort_key = render_group_factory_->prepare_render_group(
            &group,
            renderable, pass,
            i, is_blended, renderable_dist_to_camera
        );

        // Priorities run from -250 to +250, so we need to offset the index
        auto& priority_queue = priority_queues_[priority + std::abs(RENDER_PRIORITY_MIN)];
        priority_queue.insert(std::move(group), std::move(idx));
    }
}


void RenderQueue::clear() {
    thread::Lock<thread::Mutex> lock(queue_lock_);
    for(auto& queue: priority_queues_) {
        queue.clear();
    }

    renderables_.clear();
}

void RenderQueue::traverse(RenderQueueVisitor* visitor, uint64_t frame_id) const {
    thread::Lock<thread::Mutex> lock(queue_lock_);

    visitor->start_traversal(*this, frame_id, stage_);

    for(auto& queue: priority_queues_) {
        IterationType pass_iteration_type = ITERATION_TYPE_ONCE;
        MaterialPass* material_pass = nullptr, *last_pass = nullptr;

        const RenderGroup* last_group = nullptr;

        for(auto& p: queue) {
            const RenderGroup* current_group = &p.first;
            const Renderable* renderable = &renderables_[p.second];

            /* We do this here so that we don't change render group unless something in the
             * new group is visible */
            if(!last_group || *current_group != *last_group) {
                visitor->change_render_group(last_group, current_group);
            }

            material_pass = renderable->material->pass(current_group->sort_key.pass);

            if(material_pass != last_pass) {
                pass_iteration_type = material_pass->iteration_type();
                visitor->change_material_pass(last_pass, material_pass);
                last_pass = material_pass;
            }

            uint32_t iterations = 1;

            // Get any lights which are visible and affecting the renderable this frame
            auto& lights = renderable->lights_affecting_this_frame;

            if(pass_iteration_type == ITERATION_TYPE_N) {
                iterations = material_pass->max_iterations();
            } else if(pass_iteration_type == ITERATION_TYPE_ONCE_PER_LIGHT) {                
                iterations = std::distance(lights.begin(), std::find(lights.begin(), lights.end(), nullptr));
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
    }

    visitor->end_traversal(*this, stage_);
}

std::size_t RenderQueue::group_count(Pass pass_number) const {
    uint32_t i = 0;
    auto& queue = priority_queues_.at(pass_number);
    for(auto it = queue.begin(); it != queue.end(); it = queue.upper_bound(it->first)) {
        ++i;
    }

    return i;
}

}
}
