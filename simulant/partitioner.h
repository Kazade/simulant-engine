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

class StageNode;
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
    StageNode* node = nullptr;
};

#define MAX_STAGED_WRITES 1024

class Partitioner:
    public RefCounted<Partitioner>,
    public StageNode {

public:
    struct WriteSlots {
        StagedWrite slot[WRITE_OPERATION_MAX];
        uint8_t bits = 0;
    };

    Partitioner(Scene* owner, StageNodeType node_type):
        StageNode(owner, node_type) {}

    void add_stage_node(StageNode* node) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_ADD;
        write.node = node;
        stage_write(node, write);
    }

    void update_stage_node(StageNode* node, const AABB& bounds) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_UPDATE;
        write.new_bounds = bounds;
        write.node = node;
        stage_write(node, write);
    }

    void remove_stage_node(StageNode* node) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_REMOVE;
        write.node = node;
        stage_write(node, write);
    }

    void _apply_writes();

protected:
    virtual void apply_staged_write(const StageNodeID& key, const StagedWrite& write) = 0;

    void stage_write(StageNode* node, const StagedWrite& op);

    /* Partitioners take over the responsibility of deciding which
     * of their children gets rendered */
    bool _generates_renderables_for_descendents() const override {
        return true;
    }

private:
    thread::Mutex staging_lock_;

    std::vector<StageNode*> staged_writes_;
    std::unordered_map<StageNode*, StageNodeID> removed_nodes_;
};

}

namespace std {
    DEFINE_ENUM_HASH(smlt::WriteOperation);
}

#endif // PARTITIONER_H
