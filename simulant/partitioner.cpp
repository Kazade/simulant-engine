#include "partitioner.h"

namespace smlt {

void Partitioner::add_particle_system(ParticleSystemID ps) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.particle_system_id = ps;
    stage_write(write);
}

void Partitioner::remove_particle_system(ParticleSystemID ps) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.particle_system_id = ps;
    stage_write(write);
}

void Partitioner::add_geom(GeomID geom_id) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.geom_id = geom_id;
    stage_write(write);
}

void Partitioner::remove_geom(GeomID geom_id) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.geom_id = geom_id;
    stage_write(write);
}

void Partitioner::add_actor(ActorID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.actor_id = obj;
    stage_write(write);
}

void Partitioner::remove_actor(ActorID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.actor_id = obj;
    stage_write(write);
}

void Partitioner::add_light(LightID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.light_id = obj;
    stage_write(write);
}

void Partitioner::remove_light(LightID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.light_id = obj;
    stage_write(write);
}

void Partitioner::_apply_writes() {
    for(auto& write: staged_writes_) {
        apply_staged_write(write);
    }
    staged_writes_.clear();
}

void Partitioner::stage_write(const StagedWrite &op) {
    staged_writes_.push_back(op);
}

}
