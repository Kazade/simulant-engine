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

#ifndef NULL_PARTITIONER_H
#define NULL_PARTITIONER_H

#include <map>

#include "../partitioner.h"
#include "../generic/unique_id.h"

namespace smlt {

class SubActor;

class NullPartitioner : public Partitioner {
public:
    NullPartitioner(Stage* ss):
        Partitioner(ss) {}

    void lights_and_geometry_visible_from(
        CameraID camera_id,
        std::vector<LightID> &lights_out,
        std::vector<StageNode*> &geom_out
    ) override;

private:
    void apply_staged_write(const UniqueIDKey& key, const StagedWrite& write) override;

    std::map<UniqueIDKey, StageNode*> geometry_;
    std::map<UniqueIDKey, Light*> lights_;
};

}

#endif // NULL_PARTITIONER_H
