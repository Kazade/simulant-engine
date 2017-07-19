#include "partitioner.h"

namespace smlt {

void Partitioner::add_particle_system(ParticleSystemID ps) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_PARTICLE_SYSTEM;
    write.particle_system_id = ps;
    stage_write(write);
}

void Partitioner::update_particle_system(ParticleSystemID ps, const AABB &bounds) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_UPDATE;
    write.new_bounds = bounds;
    write.particle_system_id = ps;
    stage_write(write);
}

void Partitioner::remove_particle_system(ParticleSystemID ps) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_PARTICLE_SYSTEM;
    write.particle_system_id = ps;
    stage_write(write);
}

void Partitioner::add_geom(GeomID geom_id) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_GEOM;
    write.geom_id = geom_id;
    stage_write(write);
}

void Partitioner::remove_geom(GeomID geom_id) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_GEOM;
    write.geom_id = geom_id;
    stage_write(write);
}

void Partitioner::add_actor(ActorID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_ACTOR;
    write.actor_id = obj;
    stage_write(write);
}

void Partitioner::update_actor(ActorID actor_id, const AABB &bounds) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_UPDATE;
    write.new_bounds = bounds;
    write.actor_id = actor_id;
    stage_write(write);
}

void Partitioner::remove_actor(ActorID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_ACTOR;
    write.actor_id = obj;
    stage_write(write);
}

void Partitioner::add_light(LightID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_ADD;
    write.stage_node_type = STAGE_NODE_TYPE_LIGHT;
    write.light_id = obj;
    stage_write(write);
}

void Partitioner::update_light(LightID light_id, const AABB &bounds) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_UPDATE;
    write.new_bounds = bounds;
    write.light_id = light_id;
    stage_write(write);
}

void Partitioner::remove_light(LightID obj) {
    StagedWrite write;
    write.operation = WRITE_OPERATION_REMOVE;
    write.stage_node_type = STAGE_NODE_TYPE_LIGHT;
    write.light_id = obj;
    stage_write(write);
}

void Partitioner::_apply_writes() {
    std::unordered_set<ActorID> seen_actors;
    std::unordered_set<LightID> seen_lights;
    std::unordered_set<GeomID> seen_geoms;
    std::unordered_set<ParticleSystemID> seen_particle_systems;

    /* Iterate the staged writes in most-recent-first order, so we
     * only apply the last write for each element */
    while(!staged_writes_.empty()) {
        auto write = staged_writes_.top();
        staged_writes_.pop();

        if(write.stage_node_type == STAGE_NODE_TYPE_ACTOR) {
            if(seen_actors.count(write.actor_id)) {
                continue;
            } else {
                seen_actors.insert(write.actor_id);
            }
        } else if(write.stage_node_type == STAGE_NODE_TYPE_GEOM) {
            if(seen_geoms.count(write.geom_id)) {
                continue;
            } else {
                seen_geoms.insert(write.geom_id);
            }
        } else if(write.stage_node_type == STAGE_NODE_TYPE_LIGHT) {
            if(seen_lights.count(write.light_id)) {
                continue;
            } else {
                seen_lights.insert(write.light_id);
            }
        } else if(write.stage_node_type == STAGE_NODE_TYPE_PARTICLE_SYSTEM) {
            if(seen_particle_systems.count(write.particle_system_id)) {
                continue;
            } else {
                seen_particle_systems.insert(write.particle_system_id);
            }
        }

        apply_staged_write(write);
    }
}

void Partitioner::stage_write(const StagedWrite &op) {
    staged_writes_.push(op);
}

}
