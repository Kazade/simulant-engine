#pragma once


#include "../partitioner.h"
#include "./impl/spatial_hash.h"
#include "../stage.h"
#include "../generic/threading/shared_mutex.h"

namespace smlt {

enum PartitionerEntryType {
    PARTITIONER_ENTRY_TYPE_LIGHT,
    PARTITIONER_ENTRY_TYPE_RENDERABLE
};

struct PartitionerEntry : public SpatialHashEntry {
    PartitionerEntry(RenderablePtr renderable):
        type(PARTITIONER_ENTRY_TYPE_RENDERABLE),
        renderable(renderable) {}

    PartitionerEntry(LightID light_id):
        type(PARTITIONER_ENTRY_TYPE_LIGHT),
        light_id(light_id) {}

    ~PartitionerEntry() {}

    PartitionerEntryType type;
    union {
        RenderablePtr renderable;
        LightID light_id;
    };
};

class SpatialHashPartitioner : public Partitioner {
public:
    SpatialHashPartitioner(Stage* ss);
    ~SpatialHashPartitioner();

    void add_actor(ActorID obj);
    void remove_actor(ActorID obj);

    void add_geom(GeomID geom_id);
    void remove_geom(GeomID geom_id);

    void add_light(LightID obj);
    void remove_light(LightID obj);

    void add_particle_system(ParticleSystemID ps);
    void remove_particle_system(ParticleSystemID ps);

    std::vector<LightID> lights_visible_from(CameraID camera_id);
    std::vector<RenderablePtr> geometry_visible_from(CameraID camera_id);

    void event_actor_changed(ActorID ent);

private:
    SpatialHash* hash_ = nullptr;

    typedef std::shared_ptr<PartitionerEntry> PartitionerEntryPtr;

    std::unordered_map<ActorID, std::vector<PartitionerEntryPtr>> actor_entries_;
    std::unordered_map<LightID, PartitionerEntryPtr> light_entries_;
    std::unordered_map<ParticleSystemID, PartitionerEntryPtr> particle_system_entries_;

    shared_mutex lock_;
};

}
