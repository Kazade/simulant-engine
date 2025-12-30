#pragma once

#include <map>

#include "../assets/prefab.h"
#include "stage_node.h"

namespace smlt {

class PrefabInstance: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_PREFAB_INSTANCE,
                             "prefab_instance");
    S_DEFINE_STAGE_NODE_PARAM(PrefabInstance, "prefab", PrefabPtr, no_value,
                              "The prefab to spawn");

    PrefabInstance(Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    bool on_create(Params params) override;

private:
    std::map<uint32_t, StageNodePtr> build_tree(const PrefabPtr& prefab);

    StageNode* default_node_factory(StageNode* parent, const PrefabNode& input);
};

} // namespace smlt
