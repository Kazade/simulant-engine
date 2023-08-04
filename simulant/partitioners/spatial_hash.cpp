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

void SpatialHashPartitioner::stage_add_node(StageNode* obj) {
    if(!obj) {
        return;
    }

    if(auto light = dynamic_cast<Light*>(obj)) {
        if(light->type() == LIGHT_TYPE_DIRECTIONAL) {
            // Directional lights are always visible, no need to add them to the hash
            directional_lights_.insert(light->id());
        }
    }

    auto partitioner_entry = std::make_shared<PartitionerEntry>(obj);
    hash_->insert_object_for_box(obj->transformed_aabb(), partitioner_entry.get());
    node_entries_.insert(std::make_pair(obj->id(), partitioner_entry));
}

void SpatialHashPartitioner::stage_remove_node(StageNodeID node_id) {
    auto it = node_entries_.find(node_id);
    if(it != node_entries_.end()) {
        hash_->remove_object(it->second.get());
        node_entries_.erase(it);
    } else if(directional_lights_.find(node_id) != directional_lights_.end()) {
        directional_lights_.erase(node_id);
    }
}

void SpatialHashPartitioner::_update_node(const AABB &bounds, StageNodeID node_id) {
    hash_->update_object_for_box(bounds, node_entries_.at(node_id).get());
}

void SpatialHashPartitioner::apply_staged_write(const StageNodeID& key, const StagedWrite &write) {
    _apply_writes();

    if(write.operation == WRITE_OPERATION_ADD) {
        stage_add_node(write.node);
    } else if(write.operation == WRITE_OPERATION_UPDATE) {
        _update_node(write.new_bounds, key);
    } else if(write.operation == WRITE_OPERATION_REMOVE) {
        stage_remove_node(key);
    }
}

void SpatialHashPartitioner::lights_and_geometry_visible_from(
        StageNodeID camera_id, std::vector<StageNodeID> &lights_out,
        std::vector<StageNode*> &geom_out) {

    auto frustum = stage->camera(camera_id)->frustum();
    auto entries = hash_->find_objects_within_frustum(frustum);

    for(auto& entry: entries) {
        auto pentry = static_cast<PartitionerEntry*>(entry);

        // FIXME! Separate lights !
        geom_out.push_back(pentry->node_);
    }

    // Add directional lights to the end
    lights_out.insert(lights_out.end(), directional_lights_.begin(), directional_lights_.end());
}

}
