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

#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <memory>
#include <set>
#include <map>
#include <vector>

#include "generic/containers/contiguous_map.h"
#include "generic/property.h"
#include "generic/managed.h"
#include "renderers/renderer.h"
#include "types.h"
#include "interfaces.h"
#include "nodes/stage_node.h"

namespace smlt {

class SubActor;

enum WriteOperation {
    WRITE_OPERATION_ADD,
    WRITE_OPERATION_UPDATE,
    WRITE_OPERATION_REMOVE,
    WRITE_OPERATION_MAX
};

struct StagedWrite {
    WriteOperation operation;
    AABB new_bounds;
};

#define MAX_STAGED_WRITES 1024

class Partitioner:
    public RefCounted<Partitioner> {

public:
    Partitioner(Stage* ss):
        stage_(ss) {}

    void add_stage_node(StageNode* node) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_ADD;
        stage_write(node->key(), write);
    }

    void update_stage_node(StageNode* node, const AABB& bounds) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_UPDATE;
        write.new_bounds = bounds;
        stage_write(node->key(), write);
    }

    void remove_stage_node(StageNode* node) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_REMOVE;
        stage_write(node->key(), write);
    }

    void _apply_writes();

    virtual void lights_and_geometry_visible_from(
        CameraID camera_id,
        std::vector<LightID>& lights_out,
        std::vector<StageNode*>& geom_out
    ) = 0;

    virtual MeshID debug_mesh_id() { return MeshID(); }
protected:
    Stage* get_stage() const { return stage_; }

    virtual void apply_staged_write(const UniqueIDKey& key, const StagedWrite& write) = 0;

    void stage_write(const UniqueIDKey& key, const StagedWrite& op);

private:
    Stage* stage_;

    thread::Mutex staging_lock_;

    struct WriteSlots {
        StagedWrite slot[WRITE_OPERATION_MAX];
        uint8_t bits = 0;
    };

    ContiguousMap<UniqueIDKey, WriteSlots> staged_writes_;

protected:
    Property<decltype(&Partitioner::stage_)> stage = { this, &Partitioner::stage_ };

};

}

namespace std {
    DEFINE_ENUM_HASH(smlt::WriteOperation);
}

#endif // PARTITIONER_H
