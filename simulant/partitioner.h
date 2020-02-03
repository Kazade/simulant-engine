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

#include <stack>
#include <memory>
#include <set>
#include <map>
#include <vector>

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

enum StageNodeType {
    STAGE_NODE_TYPE_ACTOR,
    STAGE_NODE_TYPE_LIGHT,
    STAGE_NODE_TYPE_GEOM,
    STAGE_NODE_TYPE_PARTICLE_SYSTEM
};

struct StagedWrite {
    WriteOperation operation;
    StageNodeType stage_node_type;
    AABB new_bounds;
};


class Partitioner:
    public RefCounted<Partitioner> {

public:
    Partitioner(Stage* ss):
        stage_(ss) {}

    void add_particle_system(ParticleSystemID particle_system_id);
    void update_particle_system(ParticleSystemID particle_system_id, const AABB& bounds);
    void remove_particle_system(ParticleSystemID particle_system_id);

    void add_geom(GeomID geom_id);
    void remove_geom(GeomID geom_id);

    void add_actor(ActorID actor_id);
    void update_actor(ActorID actor_id, const AABB& bounds);
    void remove_actor(ActorID actor_id);

    void add_light(LightID light_id);
    void update_light(LightID light_id, const AABB& bounds);
    void remove_light(LightID light_id);

    void _apply_writes();

    virtual void lights_and_geometry_visible_from(
        CameraID camera_id,
        std::vector<LightID>& lights_out,
        std::vector<StageNode*>& geom_out
    ) = 0;

    virtual MeshID debug_mesh_id() { return MeshID(); }
protected:
    Property<Partitioner, Stage> stage = { this, &Partitioner::stage_ };

    Stage* get_stage() const { return stage_; }

    virtual void apply_staged_write(const UniqueIDKey& key, const StagedWrite& write) = 0;

    template<typename ID>
    void stage_write(const ID& id, const StagedWrite& op) {
        auto key = make_unique_id_key(id);
        auto& value = staged_writes_[key];
        value.slot[op.operation] = op;

        if(!(value.bits & (1 << WRITE_OPERATION_ADD)) && op.operation == WRITE_OPERATION_REMOVE) {
            /* If no write op has happened, and this was a remove operation, we store
             * that this was the first operation of the two */
            value.bits |= (1 << WRITE_OPERATION_MAX);
        }

        value.bits |= (1 << op.operation);
    }

private:
    Stage* stage_;

    thread::Mutex staging_lock_;

    struct WriteSlots {
        StagedWrite slot[WRITE_OPERATION_MAX];
        uint8_t bits = 0;
    };

    std::map<UniqueIDKey, WriteSlots> staged_writes_;
};

}

namespace std {
    DEFINE_ENUM_HASH(smlt::WriteOperation);
}

#endif // PARTITIONER_H
