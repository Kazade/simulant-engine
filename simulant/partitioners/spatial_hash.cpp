#include "spatial_hash.h"
#include "../nodes/actor.h"
#include "../nodes/light.h"
#include "../camera.h"
#include "../nodes/particles.h"

namespace smlt {

SpatialHashPartitioner::SpatialHashPartitioner(smlt::Stage *ss):
    Partitioner(ss) {

    hash_ = new SpatialHash();
}

SpatialHashPartitioner::~SpatialHashPartitioner() {
    delete hash_;
    hash_ = nullptr;
}

void SpatialHashPartitioner::add_actor(ActorID obj) {
    write_lock<shared_mutex> lock(lock_);

    auto actor = stage->actor(obj);
    actor->each([this, &obj](uint32_t i, SubActor* subactor) {
        auto partitioner_entry = std::make_shared<PartitionerEntry>(subactor->shared_from_this());
        hash_->insert_object_for_box(subactor->transformed_aabb(), partitioner_entry.get());
        actor_entries_[obj].push_back(partitioner_entry);
    });
}

void SpatialHashPartitioner::remove_actor(ActorID obj) {
    write_lock<shared_mutex> lock(lock_);

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
    write_lock<shared_mutex> lock(lock_);

    auto light = stage->light(obj);

    auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
    hash_->insert_object_for_box(light->aabb(), partitioner_entry.get());
    light_entries_[obj] = partitioner_entry;
}

void SpatialHashPartitioner::remove_light(LightID obj) {
    write_lock<shared_mutex> lock(lock_);

    auto it = light_entries_.find(obj);
    if(it != light_entries_.end()) {
        hash_->remove_object(it->second.get());
        light_entries_.erase(it);
    }
}

void SpatialHashPartitioner::add_particle_system(ParticleSystemID ps) {
    write_lock<shared_mutex> lock(lock_);

    auto particle_system = stage->particle_system(ps);
    auto partitioner_entry = std::make_shared<PartitionerEntry>(particle_system->shared_from_this());
    hash_->insert_object_for_box(particle_system->aabb(), partitioner_entry.get());
    particle_system_entries_[ps] = partitioner_entry;
}

void SpatialHashPartitioner::remove_particle_system(ParticleSystemID ps) {
    write_lock<shared_mutex> lock(lock_);

    auto it = particle_system_entries_.find(ps);
    if(it != particle_system_entries_.end()) {
        hash_->remove_object(it->second.get());
        particle_system_entries_.erase(it);
    }
}

std::vector<LightID> SpatialHashPartitioner::lights_visible_from(CameraID camera_id) {
    read_lock<shared_mutex> lock(lock_);

    std::vector<LightID> lights;

    auto entries = hash_->find_objects_within_frustum(
        stage->window->camera(camera_id)->frustum()
    );

    for(auto& entry: entries) {
        auto pentry = static_cast<PartitionerEntry*>(entry);
        if(pentry->type == PARTITIONER_ENTRY_TYPE_LIGHT) {
            lights.push_back(pentry->light_id);
        }
    }

    return lights;
}

std::vector<RenderablePtr> SpatialHashPartitioner::geometry_visible_from(CameraID camera_id) {
    read_lock<shared_mutex> lock(lock_);

    std::vector<RenderablePtr> geometry;
    auto camera = stage->window->camera(camera_id);
    auto entries = hash_->find_objects_within_frustum(camera->frustum());
    for(auto& entry: entries) {
        auto pentry = static_cast<PartitionerEntry*>(entry);
        if(pentry->type == PARTITIONER_ENTRY_TYPE_RENDERABLE) {
            geometry.push_back(pentry->renderable);
        }
    }

    return geometry;
}

void SpatialHashPartitioner::event_actor_changed(ActorID ent) {

}

}
