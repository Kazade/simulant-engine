#pragma once

#include <functional>

#include "stage_node.h"
#include "../core/memory.h"

namespace smlt {

typedef std::function<StageNode* (void*)> StageNodeConstructFunction;
typedef std::function<void (StageNode*)> StageNodeDestructFunction;


struct StageNodeTypeInfo {
    StageNodeType type;
    std::size_t size_in_bytes;
    std::size_t alignment;

    StageNodeConstructFunction constructor;
    StageNodeDestructFunction destructor;
};

class Scene;

class StageNodeManager {
private:
    template<typename T>
    static StageNode* standard_new(Scene* scene, void* mem) {
        return new (mem) T(scene);
    }

    template<typename T>
    static void standard_delete(StageNode *mem) {
        ((T*) mem)->~T();
    }

    std::unordered_map<StageNodeType, StageNodeTypeInfo> registered_nodes_;
    std::unordered_map<StageNodeID, StageNode*> all_nodes_;

protected:
    bool clean_up_node(StageNode* node) {
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

        all_nodes_.erase(node->id());

        try {
            node->clean_up();
        } catch(...) {
            S_ERROR("Exception calling node clean_up");
        }

        it->second.destructor(node);        
        free(node);
        S_DEBUG("Destroyed node with type {0} at address {1}", type, node);
        return true;
    }

    Scene* scene_;
public:
    StageNodeManager(Scene* scene):
        scene_(scene) {}

    /* Constant-time node lookup by ID */
    StageNode* get_node(StageNodeID id) const {
        return all_nodes_.at(id);
    }

    /* Constant-time node existence check */
    bool has_node(StageNodeID id) const {
        return all_nodes_.count(id) > 0;
    }

    /* Non-template API does the work for easier binding with other languages */
    StageNode* create_node(StageNodeType type, void* params) {
        auto info = registered_nodes_.find(type);
        if(info == registered_nodes_.end()) {
            S_ERROR("Unable to find registered node: {0}", type);
            return nullptr;
        }

        /* Allocate aligned memory. This is temporary, in future we'll do some
         * chunked allocation depending on the node size */
        void* mem = smlt::aligned_alloc(info->second.alignment, info->second.size_in_bytes);
        StageNode* node = info->second.constructor(mem);
        if(!node->init() || !node->_create(params)) {
            S_ERROR("Failed to initialize node");
            info->second.destructor(node);
            free(node);
            return nullptr;
        }

        S_DEBUG("Created new node of type {0} at address {1}", node->node_type(), node);
        all_nodes_.insert(std::make_pair(node->id(), node));
        return node;
    }

    template<typename T, typename... Args>
    T* create_node(Args&&... args) {
        auto params = typename T::Meta::params_type(std::forward<Args>(args)...);
        return (T*) create_node(T::Meta::node_type, &params);
    }

    bool register_stage_node(
        StageNodeType type,
        std::size_t size_in_bytes,
        std::size_t alignment,
        StageNodeConstructFunction construct_func, StageNodeDestructFunction destruct_func) {

        if(registered_nodes_.find(type) != registered_nodes_.end()) {
            S_WARN("Attempted to register duplicate node: {0}", type);
            return false;
        }

        StageNodeTypeInfo info = {
            type, size_in_bytes, alignment, construct_func, destruct_func
        };

        registered_nodes_.insert(std::make_pair(type, info));
        S_DEBUG("Registered new stage node type: {0}", type);
        return true;
    }

    template<typename T>
    bool register_stage_node() {
        return register_stage_node(
            T::Meta::node_type,
            sizeof(T), alignof(T),
            std::bind(&StageNodeManager::standard_new<T>, scene_, std::placeholders::_1),
            &StageNodeManager::standard_delete<T>
        );
    }
};

}
