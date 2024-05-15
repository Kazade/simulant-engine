#include "partitioner.h"

#include "nodes/actor.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/light.h"
#include "nodes/mesh_instancer.h"

namespace smlt {

void Partitioner::_apply_writes() {
    for(auto& p: staged_writes_) {
        for(auto& sw: p.second) {
            apply_staged_write(sw);
        }
    }

    staged_writes_.clear();
    write_count_ = 0;
}

void Partitioner::stage_write(StageNode* node, const StagedWrite& op) {
    auto it = staged_writes_.find(node);
    if(it == staged_writes_.end()) {
        staged_writes_.insert(node, std::vector<StagedWrite>());
    }

    auto& list = staged_writes_.at(node);

    /* If someone sends a list of updates, we only store the last one
     * (we update the existing entry), otherwise we add to the queue */
    if(!list.empty() && list.back().operation == op.operation) {
        list.back().new_bounds = op.new_bounds;
    } else {
        list.push_back(op);
        list.back().node = node;
        write_count_++;
    }

    /* Apply staged writes immediately to prevent the size spiralling */
    if(write_count_ >= MAX_STAGED_WRITES) {
        _apply_writes();
    }
}
}
