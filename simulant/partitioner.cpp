#include "partitioner.h"

#include "nodes/actor.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/light.h"
#include "nodes/mesh_instancer.h"

namespace smlt {

void Partitioner::_apply_writes() {
    for(auto p: staged_writes_) {
        auto it = removed_nodes_.find(p);
        if(it != removed_nodes_.end()) {
            StagedWrite write;
            write.operation = WRITE_OPERATION_REMOVE;
            apply_staged_write(it->second, write);
            removed_nodes_.erase(it);
        } else {
            auto key = p->id();
            if(p->partitioner_added_) {
                StagedWrite write;
                write.operation = WRITE_OPERATION_ADD;
                write.node = p;
                apply_staged_write(key, write);
            }

            StagedWrite write;
            write.operation = WRITE_OPERATION_UPDATE;
            write.new_bounds = p->transformed_aabb();
            write.node = p;
            apply_staged_write(key, write);
        }

        /* We always wipe this out once we've applied this node */
        p->partitioner_dirty_ = false;
        p->partitioner_added_ = false;
    }

    /* We've handled any writes, now deal with any remaining removed nodes */
    for(auto n: removed_nodes_) {
        StagedWrite write;
        write.operation = WRITE_OPERATION_REMOVE;
        apply_staged_write(n.second, write);
    }

    staged_writes_.clear();
    removed_nodes_.clear();
}

void Partitioner::stage_write(StageNode* node, const StagedWrite& op) {
    if(op.operation == WRITE_OPERATION_REMOVE) {
        removed_nodes_.insert(std::make_pair(node, node->id()));
    } else if(!node->partitioner_dirty_ || removed_nodes_.count(node)) {
        if(op.operation == WRITE_OPERATION_ADD) {
            node->partitioner_added_ = true;

            // If we re-added the node, we undo the removal
            removed_nodes_.erase(node);
        }

        /* OK, we haven't staged this node yet */
        staged_writes_.push_back(node);
        node->partitioner_dirty_ = true;
    }

    /* Apply staged writes immediately to prevent the size spiralling */
    if((staged_writes_.size() + removed_nodes_.size()) >= MAX_STAGED_WRITES) {
        _apply_writes();
    }
}


}
