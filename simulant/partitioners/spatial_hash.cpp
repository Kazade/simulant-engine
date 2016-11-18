#include "spatial_hash.h"

namespace smlt {

SpatialHashPartitioner::SpatialHashPartitioner(smlt::Stage *ss):
    Partitioner(ss) {

}

void SpatialHashPartitioner::add_actor(ActorID obj) {

}

void SpatialHashPartitioner::remove_actor(ActorID obj) {

}

void SpatialHashPartitioner::add_geom(GeomID geom_id) {

}

void SpatialHashPartitioner::remove_geom(GeomID geom_id) {

}

void SpatialHashPartitioner::add_light(LightID obj) {

}

void SpatialHashPartitioner::remove_light(LightID obj) {

}

void SpatialHashPartitioner::add_particle_system(ParticleSystemID ps) {

}

void SpatialHashPartitioner::remove_particle_system(ParticleSystemID ps) {

}

std::vector<LightID> SpatialHashPartitioner::lights_visible_from(CameraID camera_id) {

}

std::vector<RenderablePtr> SpatialHashPartitioner::geometry_visible_from(CameraID camera_id) {

}

void SpatialHashPartitioner::event_actor_changed(ActorID ent) {

}

}
