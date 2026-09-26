#pragma once

#include <map>

#include "../assets/prefab.h"
#include "stage_node.h"

namespace smlt {

class PrefabInstance: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META("prefab_instance");
    S_DEFINE_STAGE_NODE_PARAM(PrefabInstance, "prefab", PrefabPtr, no_value,
                              "The prefab to spawn");

    PrefabInstance(Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    bool on_create(Params params) override;

    /* Spawns `prefab`'s node graph as children of `parent`, without a
     * PrefabInstance around it - e.g. a gltf scene's own nodes, which
     * belong to the scene itself rather than being a reference to another
     * file. If the prefab has animations, an AnimationController mixin is
     * added to `parent` to drive them. Returns the spawned nodes keyed by
     * their prefab node id. */
    static std::map<uint32_t, StageNodePtr>
        instantiate(const PrefabPtr& prefab, StageNode* parent);

private:
    static std::map<uint32_t, StageNodePtr>
        build_tree(const PrefabPtr& prefab, StageNode* root);

    static StageNode* default_node_factory(StageNode* parent,
                                           const PrefabNode& input);
};

} // namespace smlt
