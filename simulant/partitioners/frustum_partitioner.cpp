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

#include "../stage.h"
#include "../nodes/camera.h"
#include "../nodes/actor.h"
#include "../nodes/light.h"
#include "../nodes/particle_system.h"
#include "../nodes/geom.h"

#include "frustum_partitioner.h"

namespace smlt {

void FrustumPartitioner::lights_and_geometry_visible_from(
        CameraID camera_id, std::vector<LightID> &lights_out,
        std::vector<StageNode*> &geom_out) {

    auto frustum = stage->camera(camera_id)->frustum();

    for(auto& node: stage->each_descendent()) {
        /* Check that the node is supposed to
         * be visible (otherwise we could end up doing work for nothing) */
        if(node.is_visible() && !node.is_destroyed()) {
            auto aabb = node.transformed_aabb();
            auto centre = aabb.centre();

            if(node.node_type() == STAGE_NODE_TYPE_LIGHT) {
                auto light = dynamic_cast<Light*>(&node);
                assert(light);

                if(!light->is_cullable() ||
                    frustum.intersects_sphere(centre, aabb.max_dimension())
                ) {
                    lights_out.push_back(light->id());
                }
            } else if(!node.is_cullable()) {
                /* If the culling mode is NEVER then we always return */
                geom_out.push_back(&node);
            } else if(frustum.intersects_aabb(aabb)) {
                geom_out.push_back(&node);
            }
        }
    }
}

void FrustumPartitioner::apply_staged_write(const UniqueIDKey& key, const StagedWrite &write) {
    _S_UNUSED(key);
    _S_UNUSED(write);
    // Do nothing, we don't need to!

}



}
