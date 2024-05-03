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

#include "null_partitioner.h"

namespace smlt {

void NullPartitioner::lights_and_geometry_visible_from(
    CameraID camera_id, std::vector<Light*>& lights_out,
    std::vector<StageNode*>& geom_out) {

    _S_UNUSED(camera_id);

    _apply_writes();

    lights_out.reserve(lights_.size());
    geom_out.reserve(geometry_.size());
    for(auto& node: lights_) {
        lights_out.push_back(node);
    }

    for(auto& node: geometry_) {
        geom_out.push_back(node);
    }
}

void NullPartitioner::apply_staged_write(const StagedWrite& write) {
    if(write.operation == WRITE_OPERATION_ADD) {
        if(write.node->node_type() == STAGE_NODE_TYPE_LIGHT) {
            lights_.insert((Light*)write.node);
        } else if(write.node->node_type() != STAGE_NODE_TYPE_CAMERA) {
            geometry_.insert(write.node);
        }
    } else if(write.operation == WRITE_OPERATION_REMOVE) {
        geometry_.erase(std::find_if(geometry_.begin(), geometry_.end(),
                                     [=](const StageNode* ent) -> bool {
            return ent == write.node;
        }));
        lights_.erase(std::find_if(lights_.begin(), lights_.end(),
                                   [=](const Light* ent) -> bool {
            return ent == write.node;
        }));
    } else if(write.operation == WRITE_OPERATION_UPDATE) {
        // Do nothing!
    }
}
}
