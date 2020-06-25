#include "partitioner.h"

#include "nodes/actor.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/light.h"

namespace smlt {

void Partitioner::add_particle_system(ParticleSystemID ps) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_PARTICLE_SYSTEM;
    stage_write(ps, write);
}

void Partitioner::update_particle_system(ParticleSystemID ps, const AABB &bounds) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_UPDATE;
    write.stage_node_type = STAGE_NODE_TYPE_PARTICLE_SYSTEM;
    write.new_bounds = bounds;
    stage_write(ps, write);
}

void Partitioner::remove_particle_system(ParticleSystemID ps) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_PARTICLE_SYSTEM;
    stage_write(ps, write);
}

void Partitioner::add_geom(GeomID geom_id) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_GEOM;
    stage_write(geom_id, write);
}

void Partitioner::remove_geom(GeomID geom_id) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_GEOM;
    stage_write(geom_id, write);
}

void Partitioner::add_actor(ActorID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_ACTOR;
    stage_write(obj, write);
}

void Partitioner::update_actor(ActorID actor_id, const AABB &bounds) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_UPDATE;
    write.stage_node_type = STAGE_NODE_TYPE_ACTOR;
    write.new_bounds = bounds;
    stage_write(actor_id, write);
}

void Partitioner::remove_actor(ActorID obj) {
    assert(obj);

    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_ACTOR;
    stage_write(obj, write);
}

void Partitioner::add_light(LightID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_LIGHT;
    stage_write(obj, write);
}

void Partitioner::update_light(LightID light_id, const AABB &bounds) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_UPDATE;
    write.stage_node_type = STAGE_NODE_TYPE_LIGHT;
    write.new_bounds = bounds;
    stage_write(light_id, write);
}

void Partitioner::remove_light(LightID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_LIGHT;
    stage_write(obj, write);
}

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


}
