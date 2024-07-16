#pragma once

#include "../utils/params.h"
#include "stage_node.h"
#include <functional>
#include <initializer_list>

namespace smlt {

class Params;

typedef std::function<StageNode*(void*)> StageNodeConstructFunction;
typedef std::function<void(StageNode*)> StageNodeDestructFunction;

struct StageNodeTypeInfo {
    StageNodeType type;
    const char* name;
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
        return new(mem) T(scene);
    }

    template<typename T>
    static void standard_delete(StageNode* mem) {
        T* to_delete = dynamic_cast<T*>(mem);
        to_delete->~T();
    }

    struct NodeData {
        void* alloc_base;
        StageNode* ptr;

        NodeData(void* alloc_base, StageNode* ptr) :
            alloc_base(alloc_base), ptr(ptr) {}
    };

    std::unordered_map<StageNodeType, StageNodeTypeInfo> registered_nodes_;
    std::unordered_map<StageNodeID, NodeData> all_nodes_;

protected:
    bool clean_up_node(StageNode* node);

    Scene* scene_;

public:
    StageNodeManager(Scene* scene) :
        scene_(scene) {}

    virtual ~StageNodeManager();

    /* Constant-time node lookup by ID */
    StageNode* get_node(StageNodeID id) const {
        return all_nodes_.at(id).ptr;
    }

    /* Constant-time node existence check */
    bool has_node(StageNodeID id) const {
        return all_nodes_.count(id) > 0;
    }

    /* Non-template API does the work for easier binding with other languages */
    StageNode* create_node(StageNodeType type, const Params& params);
    StageNode* create_node(const std::string& name, const Params& params);

    template<typename T>
    T* create_node(Params args) {
        return (T*)create_node(T::Meta::node_type, args);
    }

    template<typename T>
    T* create_node() {
        Params args;
        return (T*)create_node(T::Meta::node_type, args);
    }

    template<typename T, typename... Args>
    T* create_node(Args&&... args) {
        Params params;

        auto node_params = get_node_params<T>();
        params_unpack(params, node_params.begin(), node_params.end(), args...);

        return (T*)create_node(T::Meta::node_type, params);
    }

    bool register_stage_node(StageNodeType type, const char* name,
                             std::size_t size_in_bytes, std::size_t alignment,
                             StageNodeConstructFunction construct_func,
                             StageNodeDestructFunction destruct_func) {

        if(registered_nodes_.find(type) != registered_nodes_.end()) {
            S_WARN("Attempted to register duplicate node: {0}", type);
            return false;
        }

        StageNodeTypeInfo info = {type,      name,           size_in_bytes,
                                  alignment, construct_func, destruct_func};

        registered_nodes_.insert(std::make_pair(type, info));
        S_DEBUG("Registered new stage node type: {0}", type);
        return true;
    }

    template<typename T>
    bool register_stage_node() {
        return register_stage_node(T::Meta::node_type, T::Meta::name, sizeof(T),
                                   alignof(T),
                                   std::bind(&StageNodeManager::standard_new<T>,
                                             scene_, std::placeholders::_1),
                                   &StageNodeManager::standard_delete<T>);
    }
};

} // namespace smlt
