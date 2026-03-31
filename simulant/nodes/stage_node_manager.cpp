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
        S_ERROR("Tried to destroy an unknown type of node: {0} at address {1}",
                type, node);
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

    on_stage_node_erased(node);

    it->second.destructor(node);
    all_nodes_.erase(node_data_it);
    node_storage_.deallocate(node_data_it->second.alloc_base);

    S_DEBUG("Destroyed node with type {0} at address {1}", type, node);
    return true;
}

StageNodeManager::~StageNodeManager() {}

StageNode* StageNodeManager::create_node(const std::string& node_type_name,
                                         const Params& params,
                                         StageNode* base) {
    for(auto& node: registered_nodes_) {
        if(node.second.name == node_type_name) {
            return create_node(node.first, params, base);
        }
    }
    S_ERROR("Unable to find registered node: {0}", node_type_name);
    return nullptr;
}

bool StageNodeManager::register_stage_node(const Path& script,
                                           const char* class_name) {
    auto lua = smlt::get_app()->ensure_lua_ready();

    if(!lua->load_file(script)) {
        return false;
    }

    auto klass = lua->get_object(class_name);
    auto node_id =
        lua->get_integer(_F("{0}::Meta::node_id").format(class_name));
    auto name = lua->get_string(_F("{0}::Meta::name").format(class_name));
    return register_stage_node(node_id, name, 0, 0,
                               [klass](void*) -> StageNode* {},
                               [klass](StageNode*) {});
}

StageNode* StageNodeManager::create_node(StageNodeType type,
                                         const Params& params,
                                         StageNode* base) {
    auto info = registered_nodes_.find(type);
    if(info == registered_nodes_.end()) {
        S_ERROR("Unable to find registered node: {0}", type);
        return nullptr;
    }

    auto construction_data = info->second;
    auto alignment = construction_data.alignment;
    auto size = construction_data.size_in_bytes;
    auto& constructor = construction_data.constructor;
    auto& destructor = construction_data.destructor;

    void* mem = (size) ? node_storage_.allocate(size, alignment) : nullptr;
    StageNode* node = constructor(mem);

    if(!node->init()) {
        S_ERROR("Failed to initialize node");
        destructor(node);
        aligned_free(node);
        return nullptr;
    }

    if(base) {
        base->add_mixin(node);
    }

    if(!node->_create(params)) {
        S_ERROR("Failed to create the node");
        node->clean_up();
        info->second.destructor(node);
        aligned_free(node);
        return nullptr;
    }

    S_DEBUG("Created new node of type {0} at address {1}", node->node_type(),
            node);
    all_nodes_.insert(std::make_pair(node->id(), NodeData(mem, node)));
    on_stage_node_inserted(node);

    return node;
}
} // namespace smlt
