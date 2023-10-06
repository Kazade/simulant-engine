#include "stage_node_manager.h"
#include "../scenes/scene.h"

namespace smlt {

bool StageNodeManager::clean_up_node(StageNode* node) {
    if(!node->is_destroyed()) {
        S_ERROR("Attempted to cleanup node which has not been destroyed");
        return false;
    }

    auto type = node->node_type();
    auto it = registered_nodes_.find(type);
    if(it == registered_nodes_.end()) {
        S_ERROR(
            "Tried to destroy an unknown type of node: {0} at address {1}",
            type, node
            );
        return false;
    }

    auto node_data_it = all_nodes_.find(node->id());
    if(node_data_it == all_nodes_.end()) {
        S_ERROR("Unable to find node data for {0}", node->id());
        return false;
    }

    try {
        node->clean_up();
    } catch(...) {
        S_ERROR("Exception calling node clean_up");
    }

    it->second.destructor(node);
    free(node_data_it->second.alloc_base);
    all_nodes_.erase(node_data_it);
    S_DEBUG("Destroyed node with type {0} at address {1}", type, node);
    return true;
}

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
