#pragma once


#include "../partitioner.h"
#include "./impl/spatial_hash.h"
#include "../stage.h"
#include "../generic/threading/shared_mutex.h"

namespace smlt {

enum PartitionerEntryType {
    PARTITIONER_ENTRY_TYPE_LIGHT,
    PARTITIONER_ENTRY_TYPE_ACTOR,
    PARTITIONER_ENTRY_TYPE_GEOM,
    PARTITIONER_ENTRY_TYPE_PARTICLE_SYSTEM
};

struct PartitionerEntry : public SpatialHashEntry {
    PartitionerEntry(ActorID actor_id):
        type(PARTITIONER_ENTRY_TYPE_ACTOR),
        actor_id(actor_id) {}

    PartitionerEntry(LightID light_id):
        type(PARTITIONER_ENTRY_TYPE_LIGHT),
        light_id(light_id) {}

    PartitionerEntry(GeomID geom_id):
        type(PARTITIONER_ENTRY_TYPE_GEOM),
        geom_id(geom_id) {}

    PartitionerEntry(ParticleSystemID ps_id):
        type(PARTITIONER_ENTRY_TYPE_PARTICLE_SYSTEM),
        particle_system_id(ps_id) {}

    virtual ~PartitionerEntry() {}

    PartitionerEntryType type;
    union {
        ActorID actor_id;
        LightID light_id;
        GeomID geom_id;
        ParticleSystemID particle_system_id;
    };
};

class SpatialHashPartitioner : public Partitioner {
public:
    SpatialHashPartitioner(Stage* ss);
    virtual ~SpatialHashPartitioner();

    void lights_and_geometry_visible_from(
        CameraID camera_id,
        std::vector<LightID> &lights_out,
        std::vector<StageNode*> &geom_out
    );

private:
    void stage_add_actor(ActorID obj);
    void stage_remove_actor(ActorID obj);

    void stage_add_geom(GeomID geom_id);
    void stage_remove_geom(GeomID geom_id);

    void stage_add_light(LightID obj);
    void stage_remove_light(LightID obj);

    void stage_add_particle_system(ParticleSystemID ps);
    void stage_remove_particle_system(ParticleSystemID ps);

    void _update_actor(const AABB& bounds, ActorID actor);
    void _update_particle_system(const AABB& bounds, ParticleSystemID ps);
    void _update_light(const AABB& bounds, LightID light);

    void apply_staged_write(const StagedWrite& write);

    SpatialHash* hash_ = nullptr;

    typedef std::shared_ptr<PartitionerEntry> PartitionerEntryPtr;

    std::unordered_map<ActorID, PartitionerEntryPtr> actor_entries_;
    std::unordered_map<LightID, PartitionerEntryPtr> light_entries_;
    std::unordered_map<ParticleSystemID, PartitionerEntryPtr> particle_system_entries_;
    std::unordered_map<GeomID, PartitionerEntryPtr> geom_entries_;

    std::unordered_set<LightID> directional_lights_;

    shared_mutex lock_;
};

}
