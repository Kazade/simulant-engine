/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../partitioner.h"

namespace smlt {

class SubActor;

class FrustumPartitioner : public Partitioner {
public:
    FrustumPartitioner(Scene* owner):
        Partitioner(owner, STAGE_NODE_TYPE_PARTITIONER_FRUSTUM) {}

    void lights_and_geometry_visible_from(
        StageNodeID camera_id,
        std::vector<StageNodeID> &lights_out,
        std::vector<StageNode*> &geom_out
    );

private:
    void apply_staged_write(const StageNodeID& key, const StagedWrite& write);
};

}
