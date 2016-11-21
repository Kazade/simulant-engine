#include "spatial_hash.h"
#include "../actor.h"
#include "../light.h"
#include "../camera.h"
#include "../particles.h"

namespace smlt {

SpatialHashPartitioner::SpatialHashPartitioner(smlt::Stage *ss):
    Partitioner(ss) {

    hash_ = new HGSH();
}

SpatialHashPartitioner::~SpatialHashPartitioner() {
    delete hash_;
    hash_ = nullptr;
}

void SpatialHashPartitioner::add_actor(ActorID obj) {
    auto actor = stage->actor(obj);
    actor->each([this, &obj](uint32_t i, SubActor* subactor) {
        auto partitioner_entry = std::make_shared<PartitionerEntry>(subactor->shared_from_this());
        hash_->insert_object_for_box(subactor->aabb(), partitioner_entry.get());
        actor_entries_[obj].push_back(partitioner_entry);
    });
}

void SpatialHashPartitioner::remove_actor(ActorID obj) {
    auto it = actor_entries_.find(obj);
    if(it != actor_entries_.end()) {
        for(auto& entry: it->second) {
            hash_->remove_object(entry.get());
        }
    }
}

void SpatialHashPartitioner::add_geom(GeomID geom_id) {

}

void SpatialHashPartitioner::remove_geom(GeomID geom_id) {

}

void SpatialHashPartitioner::add_light(LightID obj) {
    auto light = stage->light(obj);

    auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
    hash_->insert_object_for_box(light->aabb(), partitioner_entry.get());
    light_entries_[obj] = partitioner_entry;
}

void SpatialHashPartitioner::remove_light(LightID obj) {
    auto it = light_entries_.find(obj);
    if(it != light_entries_.end()) {
        hash_->remove_object(it->second.get());
        light_entries_.erase(it);
    }
}

void SpatialHashPartitioner::add_particle_system(ParticleSystemID ps) {
    auto particle_system = stage->particle_system(ps);
    auto partitioner_entry = std::make_shared<PartitionerEntry>(particle_system->shared_from_this());
    hash_->insert_object_for_box(particle_system->aabb(), partitioner_entry.get());
    particle_system_entries_[ps] = partitioner_entry;
}

void SpatialHashPartitioner::remove_particle_system(ParticleSystemID ps) {
    auto it = particle_system_entries_.find(ps);
    if(it != particle_system_entries_.end()) {
        hash_->remove_object(it->second.get());
        particle_system_entries_.erase(it);
    }
}

std::vector<LightID> SpatialHashPartitioner::lights_visible_from(CameraID camera_id) {
    std::vector<LightID> lights;

    auto entries = hash_->find_objects_within_frustum(
        stage->window->camera(camera_id)->frustum()
    );

    for(auto& entry: entries) {
        lights.push_back(dynamic_cast<PartitionerEntry*>(entry)->light_id);
    }

    return lights;
}

std::vector<RenderablePtr> SpatialHashPartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<RenderablePtr> geometry;
    auto camera = stage->window->camera(camera_id);
    for(auto& entry: hash_->find_objects_within_frustum(camera->frustum())) {
        geometry.push_back(dynamic_cast<PartitionerEntry*>(entry)->renderable);
    }

    return geometry;
}

void SpatialHashPartitioner::event_actor_changed(ActorID ent) {

}

}
