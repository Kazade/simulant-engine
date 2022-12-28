#include "partitioner.h"

#include "nodes/actor.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/light.h"
#include "nodes/mesh_instancer.h"

namespace smlt {

void Partitioner::_apply_writes() {
    for(auto& p: staged_writes_) {
        bool remove_first = p.second.bits & (1 << WRITE_OPERATION_MAX);

        /* FIXME: This breaks if the order was update -> remove -> add */
        if(remove_first) {
            if((p.second.bits & (1 << WRITE_OPERATION_REMOVE))) {
                apply_staged_write(p.first, p.second.slot[WRITE_OPERATION_REMOVE]);
            }

            if((p.second.bits & (1 << WRITE_OPERATION_ADD))) {
                apply_staged_write(p.first, p.second.slot[WRITE_OPERATION_ADD]);
            }

            if((p.second.bits & (1 << WRITE_OPERATION_UPDATE))) {
                apply_staged_write(p.first, p.second.slot[WRITE_OPERATION_UPDATE]);
            }
        } else {
            if((p.second.bits & (1 << WRITE_OPERATION_ADD))) {
                apply_staged_write(p.first, p.second.slot[WRITE_OPERATION_ADD]);
            }

            if((p.second.bits & (1 << WRITE_OPERATION_UPDATE))) {
                apply_staged_write(p.first, p.second.slot[WRITE_OPERATION_UPDATE]);
            }

            if((p.second.bits & (1 << WRITE_OPERATION_REMOVE))) {
                apply_staged_write(p.first, p.second.slot[WRITE_OPERATION_REMOVE]);
            }
        }
    }

    staged_writes_.clear();
}

void Partitioner::stage_write(const UniqueIDKey& key, const StagedWrite& op) {
    staged_writes_.insert(key, WriteSlots());

    auto& value = staged_writes_.at(key);
    value.slot[op.operation] = op;

    if(!(value.bits & (1 << WRITE_OPERATION_ADD)) && op.operation == WRITE_OPERATION_REMOVE) {
        /* If no write op has happened, and this was a remove operation, we store
             * that this was the first operation of the two */
        value.bits |= (1 << WRITE_OPERATION_MAX);
    }

    value.bits |= (1 << op.operation);

    /* Apply staged writes immediately to prevent the size spiralling */
    if(staged_writes_.size() >= MAX_STAGED_WRITES) {
        _apply_writes();
    }
}


}
