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

#include "frustum_culler.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../nodes/geom.h"
#include "../nodes/light.h"
#include "../nodes/particle_system.h"
#include "../nodes/stage_node_iterators.h"
#include "../stage.h"

namespace smlt {

void FrustumCuller::do_generate_renderables(batcher::RenderQueue* render_queue,
                                            const Camera* camera,
                                            const Viewport* viewport,
                                            const DetailLevel detail_level,
                                            Light** lights,
                                            const std::size_t light_count,
                                            bool respect_visibility) {

    _apply_writes();

    auto& frustum = camera->frustum();

    for(StageNode& node: each_descendent()) {
        if(node.is_destroyed()) continue;
        // When we're respecting visibility, skip hidden subtrees up-front to
        // avoid the per-node transformed_aabb()/frustum work. Otherwise (the
        // ShadowCaster path) we let every descendant through and rely on each
        // producer to mark the resulting Renderables is_visible=false so the
        // renderer skips them at draw time.
        if(respect_visibility && !node.is_visible()) continue;

        auto aabb = node.transformed_aabb();
        if(!node.is_cullable() || frustum.intersects_aabb(aabb)) {
            node.generate_renderables(render_queue, camera, viewport,
                                      detail_level, lights, light_count,
                                      respect_visibility);
        }
    }
}

void FrustumCuller::apply_staged_write(const StagedWrite& write) {
    _S_UNUSED(write);
    // Do nothing, we don't need to!
}
}
