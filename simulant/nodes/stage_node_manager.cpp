#include "stage_node_manager.h"
#include "../scenes/scene.h"

namespace smlt {

StageNodeManager::~StageNodeManager() {

}

StageNode* StageNodeManager::create_node(StageNodeType type, void* params) {
    auto info = registered_nodes_.find(type);
    if(info == registered_nodes_.end()) {
        S_ERROR("Unable to find registered node: {0}", type);
        return nullptr;
    }

    /* Allocate aligned memory. This is temporary, in future we'll do some
         * chunked allocation depending on the node size */
    void* mem = smlt::aligned_alloc(info->second.alignment, info->second.size_in_bytes);
    StageNode* node = info->second.constructor(mem);

    fprintf(stderr, "Alloc'd: 0x%x\n", node);
    if(!node->init()) {
        S_ERROR("Failed to initialize node");
        info->second.destructor(node);
        free(node);
        return nullptr;
    }

    if(!node->_create(params)) {
        S_ERROR("Failed to create the node");
        info->second.destructor(node);
        free(node);
        return nullptr;
    }

    /* For now, when we create nodes we always parent them to the scene by default */
    node->set_parent(scene_);

    S_DEBUG("Created new node of type {0} at address {1}", node->node_type(), node);
    all_nodes_.insert(std::make_pair(node->id(), NodeData(mem, node)));

    return node;
}

}
