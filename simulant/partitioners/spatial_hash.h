#pragma once


#include "../partitioner.h"
#include "./impl/spatial_hash.h"
#include "../threads/shared_mutex.h"

namespace smlt {

struct PartitionerEntry : public SpatialHashEntry {
    PartitionerEntry(StageNode* node):
        node_(node) {}

    virtual ~PartitionerEntry() {}

    StageNode* node_;
};

class SpatialHashPartitioner : public Partitioner {
public:
    SpatialHashPartitioner(Stage* ss);
    virtual ~SpatialHashPartitioner();

    void lights_and_geometry_visible_from(
        StageNodeID camera_id,
        std::vector<StageNodeID> &lights_out,
        std::vector<StageNode*> &geom_out
    ) override;

private:
    void stage_add_node(StageNode* obj);
    void stage_remove_node(StageNodeID node_id);
    void _update_node(const AABB& bounds, StageNodeID node_id);
    void apply_staged_write(const StageNodeID& key, const StagedWrite& write) override;

    SpatialHash* hash_ = nullptr;

    typedef std::shared_ptr<PartitionerEntry> PartitionerEntryPtr;

    std::unordered_map<StageNodeID, PartitionerEntryPtr> node_entries_;
    std::unordered_set<StageNodeID> directional_lights_;
};

}
