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
        CameraID camera_id, std::vector<LightID> &lights_out,
        std::vector<StageNode*> &geom_out) {

    _S_UNUSED(camera_id);

    _apply_writes();

    lights_out.reserve(lights_.size());
    geom_out.reserve(geometry_.size());
    for(auto& node: lights_) {
        lights_out.push_back(node.second->id());
    }

    for(auto& node: geometry_) {
        geom_out.push_back(node.second);
    }
}

void NullPartitioner::apply_staged_write(const UniqueIDKey& key, const StagedWrite &write) {
    if(write.operation == WRITE_OPERATION_ADD) {
        if(key.first == typeid(Light)) {
            assert(write.node);
            lights_.insert(std::make_pair(key, (Light*) write.node));
        } else if(key.first == typeid(Camera)) {
            // Skip cameras
        } else {
            assert(write.node);
            geometry_.insert(std::make_pair(key, write.node));
        }
    } else if(write.operation == WRITE_OPERATION_REMOVE) {
        geometry_.erase(key);
        lights_.erase(key);
    } else if(write.operation == WRITE_OPERATION_UPDATE) {
        // Do nothing!
    }
}



}
