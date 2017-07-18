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

void SpatialHashPartitioner::stage_add_actor(ActorID obj) {
    write_lock<shared_mutex> lock(lock_);

    auto actor = stage->actor(obj);

    auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
    hash_->insert_object_for_box(actor->bounds(), partitioner_entry.get());
    actor_entries_.insert(std::make_pair(obj, partitioner_entry));

    actor_updates_[obj] = actor->signal_bounds_updated().connect(
        std::bind(&SpatialHashPartitioner::_update_actor, this, std::placeholders::_1, obj)
    );
}

void SpatialHashPartitioner::stage_remove_actor(ActorID obj) {
    write_lock<shared_mutex> lock(lock_);

    auto it = actor_entries_.find(obj);
    if(it != actor_entries_.end()) {
        hash_->remove_object(it->second.get());
        actor_entries_.erase(it);
    }

    actor_updates_[obj].disconnect();
    actor_updates_.erase(obj);
}

void SpatialHashPartitioner::_update_actor(const AABB &bounds, ActorID actor) {
    write_lock<shared_mutex> lock(lock_);
    hash_->update_object_for_box(bounds, actor_entries_.at(actor).get());
}

void SpatialHashPartitioner::_update_particle_system(const AABB& bounds, ParticleSystemID ps) {
    write_lock<shared_mutex> lock(lock_);
    hash_->update_object_for_box(bounds, particle_system_entries_.at(ps).get());
}

void SpatialHashPartitioner::_update_light(const AABB& bounds, LightID light) {
    write_lock<shared_mutex> lock(lock_);
    hash_->update_object_for_box(bounds, light_entries_.at(light).get());
}

void SpatialHashPartitioner::stage_add_geom(GeomID geom_id) {

}

void SpatialHashPartitioner::stage_remove_geom(GeomID geom_id) {

}

void SpatialHashPartitioner::stage_add_light(LightID obj) {
    write_lock<shared_mutex> lock(lock_);

    auto light = stage->light(obj);

    if(light->type() == LIGHT_TYPE_DIRECTIONAL) {
        // Directional lights are always visible, no need to add them to the hash
        directional_lights_.insert(obj);
    } else {
        auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
        hash_->insert_object_for_box(light->bounds(), partitioner_entry.get());
        light_entries_[obj] = partitioner_entry;

        light_updates_[obj] = light->signal_bounds_updated().connect(
            std::bind(&SpatialHashPartitioner::_update_light, this, std::placeholders::_1, obj)
        );
    }
}

void SpatialHashPartitioner::stage_remove_light(LightID obj) {
    write_lock<shared_mutex> lock(lock_);

    if(directional_lights_.find(obj) != directional_lights_.end()) {
        directional_lights_.erase(obj);
    } else {
        auto it = light_entries_.find(obj);
        if(it != light_entries_.end()) {
            hash_->remove_object(it->second.get());
            light_entries_.erase(it);

            light_updates_[obj].disconnect();
            light_updates_.erase(obj);
        }
    }
}

void SpatialHashPartitioner::stage_add_particle_system(ParticleSystemID ps) {
    write_lock<shared_mutex> lock(lock_);

    auto particle_system = stage->particle_system(ps);
    auto partitioner_entry = std::make_shared<PartitionerEntry>(ps);
    hash_->insert_object_for_box(particle_system->bounds(), partitioner_entry.get());
    particle_system_entries_[ps] = partitioner_entry;

    particle_system_updates_[ps] = particle_system->signal_bounds_updated().connect(
        std::bind(&SpatialHashPartitioner::_update_particle_system, this, std::placeholders::_1, ps)
    );
}

void SpatialHashPartitioner::stage_remove_particle_system(ParticleSystemID ps) {
    write_lock<shared_mutex> lock(lock_);

    auto it = particle_system_entries_.find(ps);
    if(it != particle_system_entries_.end()) {
        hash_->remove_object(it->second.get());
        particle_system_entries_.erase(it);
    }

    particle_system_updates_[ps].disconnect();
    particle_system_updates_.erase(ps);
}

void SpatialHashPartitioner::apply_staged_write(const StagedWrite &write) {
    if(write.operation == WRITE_OPERATION_ADD) {
        if(write.actor_id) {
            stage_add_actor(write.actor_id);
        } else if(write.geom_id) {
            stage_add_geom(write.geom_id);
        } else if(write.light_id) {
            stage_add_light(write.light_id);
        } else if(write.particle_system_id) {
            stage_add_particle_system(write.particle_system_id);
        }
    } else if(write.operation == WRITE_OPERATION_REMOVE) {
        if(write.actor_id) {
            stage_remove_actor(write.actor_id);
        } else if(write.geom_id) {
            stage_remove_geom(write.geom_id);
        } else if(write.light_id) {
            stage_remove_light(write.light_id);
        } else if(write.particle_system_id) {
            stage_remove_particle_system(write.particle_system_id);
        }
    } else if(write.operation == WRITE_OPERATION_UPDATE) {
        // Do nothing!
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

    // Add directional lights to the end
    lights.insert(lights.end(), directional_lights_.begin(), directional_lights_.end());

    return lights;
}

std::vector<RenderablePtr> SpatialHashPartitioner::geometry_visible_from(CameraID camera_id) {
    read_lock<shared_mutex> lock(lock_);

    std::vector<RenderablePtr> geometry;
    auto camera = stage->window->camera(camera_id);
    auto entries = hash_->find_objects_within_frustum(camera->frustum());
    for(auto& entry: entries) {
        auto pentry = static_cast<PartitionerEntry*>(entry);
        switch(pentry->type) {
        case PARTITIONER_ENTRY_TYPE_ACTOR: {
                ActorPtr actor = pentry->actor_id.fetch();
                actor->each([&geometry](uint32_t i, SubActor* subactor) {
                    geometry.push_back(subactor->shared_from_this());
                });
            }
            break;
            case PARTITIONER_ENTRY_TYPE_GEOM:
                assert(0 && "Not implemented");
            break;
            case PARTITIONER_ENTRY_TYPE_PARTICLE_SYSTEM:
                geometry.push_back(pentry->particle_system_id.fetch()->shared_from_this());
            break;
        default:
            break;

        }
    }

    return geometry;
}

}
