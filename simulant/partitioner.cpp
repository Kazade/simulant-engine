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
    write.stage_node_type = STAGE_NODE_TYPE_PARTICLE_SYSTEM;
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
    write.stage_node_type = STAGE_NODE_TYPE_ACTOR;
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
    write.stage_node_type = STAGE_NODE_TYPE_LIGHT;
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
    /* Static to avoid continuous construction */
    static std::unordered_map<ActorID, std::unordered_set<WriteOperation>> seen_actors;
    static std::unordered_map<LightID, std::unordered_set<WriteOperation>> seen_lights;
    static std::unordered_map<GeomID, std::unordered_set<WriteOperation>> seen_geoms;
    static std::unordered_map<ParticleSystemID, std::unordered_set<WriteOperation>> seen_particle_systems;

    seen_actors.clear();
    seen_lights.clear();
    seen_geoms.clear();
    seen_particle_systems.clear();

    std::stack<StagedWrite> filtered;

    {
        std::lock_guard<std::mutex> lock(staging_lock_);

        /* Filter the list of operations so only the last operation of each
         * type for each entity is applied. It's a stack so we iterate in most-recent-first
         * and then push onto another stack (so the newest is at the bottom) then we iterate
         * the filtered stack so they apply in the right order */
        while(!staged_writes_.empty()) {
            auto write = staged_writes_.top();
            staged_writes_.pop();

            if(write.stage_node_type == STAGE_NODE_TYPE_ACTOR) {
                if(seen_actors.count(write.actor_id) && seen_actors.at(write.actor_id).count(write.operation)) {
                    // We've already seen this operation for this actor
                    continue;
                } else {
                    seen_actors[write.actor_id].insert(write.operation);
                    filtered.push(write);
                }
            } else if(write.stage_node_type == STAGE_NODE_TYPE_LIGHT) {
                if(seen_lights.count(write.light_id) && seen_lights.at(write.light_id).count(write.operation)) {
                    // We've already seen this operation for this light
                    continue;
                } else {
                    seen_lights[write.light_id].insert(write.operation);
                    filtered.push(write);
                }
            } else if(write.stage_node_type == STAGE_NODE_TYPE_GEOM) {
                if(seen_geoms.count(write.geom_id) && seen_geoms.at(write.geom_id).count(write.operation)) {
                    // We've already seen this operation for this geom
                    continue;
                } else {
                    seen_geoms[write.geom_id].insert(write.operation);
                    filtered.push(write);
                }
            } else if(write.stage_node_type == STAGE_NODE_TYPE_PARTICLE_SYSTEM) {
                if(seen_particle_systems.count(write.particle_system_id) && seen_particle_systems.at(write.particle_system_id).count(write.operation)) {
                    // We've already seen this operation for this particle system
                    continue;
                } else {
                    seen_particle_systems[write.particle_system_id].insert(write.operation);
                    filtered.push(write);
                }
            }
        }
    }

    while(!filtered.empty()) {
        auto write = filtered.top();
        filtered.pop();
        apply_staged_write(write);
    }
}

void Partitioner::stage_write(const StagedWrite &op) {
    std::lock_guard<std::mutex> lock(staging_lock_);

    staged_writes_.push(op);
}

}
