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

#include "render_queue.h"
#include "../../assets/material.h"
#include "../../nodes/actor.h"
#include "../../nodes/camera.h"
#include "../../nodes/geom.h"
#include "../../nodes/geoms/geom_culler.h"
#include "../../nodes/particle_system.h"
#include "../../partitioner.h"
#include "../../stage.h"
#include "../../utils/float.h"

namespace smlt {
namespace batcher {

const static float f = 512;

static uint16_t pack_distance(float d) {
    const int depth = 10; // 10 bits
    const float max = (1 << depth) - 1;

    return clamp(max * (d / f), 0, 1023);
}

RenderGroupKey generate_render_group_key(
    const RenderPriority priority, const uint8_t pass, const bool is_blended,
    const float distance_to_camera, int16_t precedence, uint16_t texture_id) {

    auto dist = std::abs(distance_to_camera);
    texture_id = clamp(texture_id, 0, 1024);

    RenderGroupKey key;
    key.s.priority =
        clamp(priority + RENDER_PRIORITY_ABSOLUTE_FOREGROUND, 0, 50);
    key.s.pass = clamp(pass, 0, MAX_MATERIAL_PASSES);
    key.s.is_blended = is_blended;
    key.s.distance_to_camera =
        (is_blended) ? 1023 - pack_distance(dist) : pack_distance(dist);
    key.s.precedence = precedence;
    key.s.texture = texture_id;
    return key;
}

RenderQueue::RenderQueue() {

}

void RenderQueue::reset(StageNode* stage, RenderGroupFactory* factory, CameraPtr camera) {
    stage_node_ = stage;
    render_group_factory_ = factory;
    camera_ = camera;

    clear();
}

void RenderQueue::insert_renderable(Renderable&& renderable) {
    /*
     * Adds a renderable to the correct render groups. This goes through the
     * material passes on the renderable, calculates the render group for each one
     * and then adds the renderable to that pass's render queue
     */

    assert(stage_node_);
    assert(camera_);
    assert(render_group_factory_);

    if(!renderable.is_visible) {
        return;
    }

    if(!renderable.vertex_range_count && !renderable.index_element_count &&
       !renderable.vertex_data->count()) {
        return;
    }

    auto material = renderable.material;
    assert(material);

    auto plane =
        Plane(camera_->transform->forward(), camera_->transform->position());

    auto pos = renderable.center;
    auto renderable_dist_to_camera = plane.distance_to(pos);
    auto priority = renderable.render_priority;

    auto pass_count = material->pass_count();
    for(auto i = 0u; i < pass_count; ++i) {
        MaterialPass* pass = material->pass(i);
        RenderGroup group;

        bool is_blended = pass->is_blending_enabled();

        group.sort_key = render_group_factory_->prepare_render_group(
            &group, &renderable, pass, priority, i, is_blended,
            renderable_dist_to_camera,
            material->base_color_map()
                ? clamp(material->base_color_map()->_renderer_specific_id(), 0,
                        1024)
                : 0);

        render_queue_.insert(group, std::move(renderable));
    }
}

void RenderQueue::clear() {
    thread::Lock<thread::Mutex> lock(queue_lock_);
    render_queue_.clear();
}

void RenderQueue::traverse(RenderQueueVisitor* visitor, uint64_t frame_id) const {
    thread::Lock<thread::Mutex> lock(queue_lock_);

    visitor->start_traversal(*this, frame_id, stage_node_);

    IterationType pass_iteration_type = ITERATION_TYPE_ONCE;
    MaterialPass *material_pass = nullptr, *last_pass = nullptr;

    const RenderGroup* last_group = nullptr;

    for(auto& p: render_queue_) {
        const RenderGroup* current_group = &p.first;
        const Renderable* renderable = &p.second;

        /* We do this here so that we don't change render group unless something
         * in the new group is visible */
        if(!last_group || *current_group != *last_group) {
            visitor->change_render_group(last_group, current_group);
        }

        material_pass =
            renderable->material->pass(current_group->sort_key.s.pass);

        if(material_pass != last_pass) {
            pass_iteration_type = material_pass->iteration_type();
            visitor->change_material_pass(last_pass, material_pass);
            last_pass = material_pass;
        }

        uint32_t iterations = 1;

        // Get any lights which are visible and affecting the renderable this
        // frame
        auto& lights = renderable->lights_affecting_this_frame;

        if(pass_iteration_type == ITERATION_TYPE_N) {
            iterations = material_pass->max_iterations();
        } else if(pass_iteration_type == ITERATION_TYPE_ONCE_PER_LIGHT) {
            iterations = renderable->light_count;
        }

        for(Iteration i = 0; i < iterations; ++i) {
            LightPtr next = nullptr;

            // Pass down the light if necessary, otherwise just pass nullptr
            if(i < renderable->light_count) {
                next = lights[i];
            } else {
                next = nullptr;
            }

            if(pass_iteration_type == ITERATION_TYPE_ONCE_PER_LIGHT) {
                visitor->apply_lights(&next, 1);
            } else if(pass_iteration_type == ITERATION_TYPE_N ||
                      pass_iteration_type == ITERATION_TYPE_ONCE) {
                visitor->apply_lights(&lights[0],
                                      (uint8_t)renderable->light_count);
            }
            visitor->visit(renderable, material_pass, i);
        }

        last_group = current_group;
    }

    visitor->end_traversal(*this, stage_node_);
}

Renderable* RenderQueue::renderable(std::size_t idx) {
    std::size_t i = 0;
    for(auto& r: render_queue_) {
        if(i == idx) {
            return &r.second;
        }
        i++;
    }

    return nullptr;
}
}
}
