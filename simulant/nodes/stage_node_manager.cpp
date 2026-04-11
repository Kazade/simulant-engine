#include "stage_node_manager.h"
#include "../application.h"
#include "../scenes/scene.h"
#include "../scripting/lua/interpreter.h"

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

bool StageNodeManager::register_stage_node(const char* script_data,
                                           const char* class_name) {
    auto lua = smlt::get_app()->ensure_lua_ready();

    if(!lua->load_string(script_data)) {
        return false;
    }

    auto maybe_klass = lua->get_global(class_name);
    if(!maybe_klass) {
        S_ERROR("Unable to find specified class in Lua file: {0}", class_name);
        return false;
    }

    auto klass = maybe_klass.value();
    luabridge::LuaRef meta = klass["Meta"];
    StageNodeType node_id = meta["node_type"].cast<uint32_t>().valueOr(0);
    std::string name = meta["name"];

    // Capture the raw lua_State* and the class name string rather than a
    // luabridge::LuaRef.  LuaRef holds a Lua registry reference and calls
    // luaL_unref in its destructor; if the lambda is destroyed after
    // lua_close() (which happens when StageNodeManager::~StageNodeManager
    // runs after the interpreter has been torn down), that luaL_unref call
    // writes into already-freed Lua memory and corrupts the heap.
    // A raw pointer drop and a std::string drop are both no-ops, so
    // destroying the lambda after lua_close() is safe.
    lua_State* L = lua->lua_state();
    std::string klass_name(class_name);

    return register_stage_node(node_id, name.c_str(), sizeof(LuaStageNode),
                               alignof(LuaStageNode),
                               [L, klass_name, this](void* mem) -> StageNode* {
        // Look the class up by name from the Lua globals each time a node
        // is constructed.  This is only ever called during normal operation
        // (never during teardown), so the state is always open here.
        lua_getglobal(L, klass_name.c_str());
        if(lua_isnil(L, -1)) {
            lua_pop(L, 1);
            S_ERROR("Lua class not found: {0}", klass_name);
            return nullptr;
        }
        luabridge::LuaRef klass = luabridge::LuaRef::fromStack(L, -1);
        lua_pop(L, 1);

        auto constructor = klass["new"];
        try {
            luabridge::LuaRef instance =
                *constructor.call<luabridge::LuaRef>(klass, scene_);
            return new(mem) LuaStageNode(instance);
        } catch(const std::exception& e) {
            S_ERROR("Lua error: {0}", e.what());
            return nullptr;
        }
    }, [](StageNode* node) {
        // The node was constructed with placement new into slab-allocated
        // memory.  Calling delete would invoke operator delete on that slab
        // pointer, freeing it through the system allocator and causing a
        // double-free when the slab deallocator later tries to reclaim the
        // same block.  Call only the destructor, exactly like standard_delete.
        // No LuaRef is captured here for the same reason as above.
        static_cast<LuaStageNode*>(node)->~LuaStageNode();
    });
}

bool StageNodeManager::register_stage_node(const Path& script,
                                           const char* class_name) {
    auto lua = smlt::get_app()->ensure_lua_ready();

    if(!lua->load_file(script)) {
        return false;
    }

    auto klass = lua->get_global(class_name);
    if(!klass) {
        S_ERROR("Unable to find specified class in Lua file: {0}", class_name);
        return false;
    }

    auto node_id = 1;
    return false;
    // return register_stage_node(node_id, name, 0, 0,
    //                            [klass](void*) -> StageNode* {},
    //                            [klass](StageNode*) {});
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
