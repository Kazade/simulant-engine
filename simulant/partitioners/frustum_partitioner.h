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

struct FrustumPartitionerParams {};

class FrustumPartitioner : public Partitioner {
public:
    struct Meta {
        typedef FrustumPartitionerParams params_type;
        const static StageNodeType node_type = STAGE_NODE_TYPE_PARTITIONER_FRUSTUM;
    };

    FrustumPartitioner(Scene* owner):
        Partitioner(owner, STAGE_NODE_TYPE_PARTITIONER_FRUSTUM) {}

    const AABB& aabb() const {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(void*) override {
        return true;
    }

    void do_generate_renderables(
        batcher::RenderQueue* render_queue,
        const Camera*, const Viewport* viewport,
        const DetailLevel detail_level
    ) override;


    void apply_staged_write(const StageNodeID& key, const StagedWrite& write);
};

}
