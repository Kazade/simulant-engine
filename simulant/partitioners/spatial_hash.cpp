#include "spatial_hash.h"
#include "../nodes/actor.h"
#include "../nodes/light.h"
#include "../nodes/camera.h"
#include "../nodes/particle_system.h"
#include "../nodes/geom.h"
#include "../nodes/geoms/geom_culler.h"
#include "../stage.h"

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
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto actor = stage->actor(obj);
    if(!actor) {
        return;
    }

    auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
    hash_->insert_object_for_box(actor->transformed_aabb(), partitioner_entry.get());
    actor_entries_.insert(std::make_pair(obj, partitioner_entry));

}

void SpatialHashPartitioner::stage_remove_actor(ActorID obj) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto it = actor_entries_.find(obj);
    if(it != actor_entries_.end()) {
        hash_->remove_object(it->second.get());
        actor_entries_.erase(it);
    }
}

void SpatialHashPartitioner::_update_actor(const AABB &bounds, ActorID actor) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);
    hash_->update_object_for_box(bounds, actor_entries_.at(actor).get());
}

void SpatialHashPartitioner::_update_particle_system(const AABB& bounds, ParticleSystemID ps) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);
    hash_->update_object_for_box(bounds, particle_system_entries_.at(ps).get());
}

void SpatialHashPartitioner::_update_light(const AABB& bounds, LightID light) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);
    hash_->update_object_for_box(bounds, light_entries_.at(light).get());
}

void SpatialHashPartitioner::stage_add_geom(GeomID geom_id) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto geom = stage->geom(geom_id);

    if(!geom) {
        return;
    }

    auto partitioner_entry = std::make_shared<PartitionerEntry>(geom_id);
    hash_->insert_object_for_box(geom->transformed_aabb(), partitioner_entry.get());
    geom_entries_.insert(std::make_pair(geom_id, partitioner_entry));
}

void SpatialHashPartitioner::stage_remove_geom(GeomID geom_id) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto it = geom_entries_.find(geom_id);
    if(it != geom_entries_.end()) {
        hash_->remove_object(it->second.get());
        geom_entries_.erase(it);
    }
}

void SpatialHashPartitioner::stage_add_light(LightID obj) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto light = stage->light(obj);

    if(!light) {
        return;
    }

    if(light->type() == LIGHT_TYPE_DIRECTIONAL) {
        // Directional lights are always visible, no need to add them to the hash
        directional_lights_.insert(obj);
    } else {
        auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
        hash_->insert_object_for_box(light->transformed_aabb(), partitioner_entry.get());
        light_entries_[obj] = partitioner_entry;
    }
}

void SpatialHashPartitioner::stage_remove_light(LightID obj) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    if(directional_lights_.find(obj) != directional_lights_.end()) {
        directional_lights_.erase(obj);
    } else {
        auto it = light_entries_.find(obj);
        if(it != light_entries_.end()) {
            hash_->remove_object(it->second.get());
            light_entries_.erase(it);
        }
    }
}

void SpatialHashPartitioner::stage_add_particle_system(ParticleSystemID ps) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto particle_system = stage->particle_system(ps);
    auto partitioner_entry = std::make_shared<PartitionerEntry>(ps);
    hash_->insert_object_for_box(particle_system->transformed_aabb(), partitioner_entry.get());
    particle_system_entries_[ps] = partitioner_entry;
}

void SpatialHashPartitioner::stage_remove_particle_system(ParticleSystemID ps) {
    thread::WriteLock<thread::SharedMutex> lock(lock_);

    auto it = particle_system_entries_.find(ps);
    if(it != particle_system_entries_.end()) {
        hash_->remove_object(it->second.get());
        particle_system_entries_.erase(it);
    }
}

void SpatialHashPartitioner::apply_staged_write(const UniqueIDKey& key, const StagedWrite &write) {
    bool is_actor = key.first == typeid(Actor);
    bool is_geom = key.first == typeid(Geom);
    bool is_light = key.first == typeid(Light);
    bool is_ps = key.first == typeid(ParticleSystem);

    if(is_actor) {
        auto aid = make_unique_id_from_key<ActorID>(key);
        if(write.operation == WRITE_OPERATION_ADD) {
            stage_add_actor(aid);
        } else if(write.operation == WRITE_OPERATION_UPDATE) {
            _update_actor(write.new_bounds, aid);
        } else if(write.operation == WRITE_OPERATION_REMOVE) {
            stage_remove_actor(aid);
        }
    } else if(is_geom) {
        auto aid = make_unique_id_from_key<GeomID>(key);
        if(write.operation == WRITE_OPERATION_ADD) {
            stage_add_geom(aid);
        } else if(write.operation == WRITE_OPERATION_REMOVE) {
            stage_remove_geom(aid);
        }
    } else if(is_light) {
        auto lid = make_unique_id_from_key<LightID>(key);
        if(write.operation == WRITE_OPERATION_ADD) {
            stage_add_light(lid);
        } else if(write.operation == WRITE_OPERATION_UPDATE) {
            _update_light(write.new_bounds, lid);
        } else if(write.operation == WRITE_OPERATION_REMOVE) {
            stage_remove_light(lid);
        }
    } else if(is_ps) {
        auto pid = make_unique_id_from_key<ParticleSystemID>(key);
        if(write.operation == WRITE_OPERATION_ADD) {
            stage_add_particle_system(pid);
        } else if(write.operation == WRITE_OPERATION_UPDATE) {
            _update_particle_system(write.new_bounds, pid);
        } else if(write.operation == WRITE_OPERATION_REMOVE) {
            stage_remove_particle_system(pid);
        }
    } else {
        assert(0 && "Not implemented");
    }
}

void SpatialHashPartitioner::lights_and_geometry_visible_from(
        CameraID camera_id, std::vector<LightID> &lights_out,
        std::vector<StageNode*> &geom_out) {

    thread::ReadLock<thread::SharedMutex> lock(lock_);

    auto frustum = stage->camera(camera_id)->frustum();
    auto entries = hash_->find_objects_within_frustum(frustum);

    for(auto& entry: entries) {
        auto pentry = static_cast<PartitionerEntry*>(entry);

        switch(pentry->type) {
        case PARTITIONER_ENTRY_TYPE_ACTOR: {
                ActorPtr actor = pentry->actor_id.fetch();
                geom_out.push_back(actor);
            }
            break;
            case PARTITIONER_ENTRY_TYPE_GEOM: {
                auto geom = pentry->geom_id.fetch();
                geom_out.push_back(geom);
            } break;
            case PARTITIONER_ENTRY_TYPE_PARTICLE_SYSTEM:
                geom_out.push_back(pentry->particle_system_id.fetch());
            break;
            case PARTITIONER_ENTRY_TYPE_LIGHT:
                lights_out.push_back(pentry->light_id);
            break;
        default:
            break;

        }
    }

    // Add directional lights to the end
    lights_out.insert(lights_out.end(), directional_lights_.begin(), directional_lights_.end());
}

}
