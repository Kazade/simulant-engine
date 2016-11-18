#pragma once


#include "../partitioner.h"
#include "./impl/hgsh.h"
#include "../stage.h"

namespace smlt {

class SpatialHashPartitioner : public Partitioner {
public:
    SpatialHashPartitioner(Stage* ss);

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
};

}
