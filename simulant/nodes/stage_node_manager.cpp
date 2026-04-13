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

    void* alloc_base = node_data_it->second.alloc_base;
    it->second.destructor(node);
    all_nodes_.erase(node_data_it);
    node_storage_.deallocate(alloc_base);

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

    return register_stage_node_from_lua_state(lua->lua_state(), class_name);
}

bool StageNodeManager::register_stage_node(const Path& script,
                                           const char* class_name) {
    auto lua = smlt::get_app()->ensure_lua_ready();

    if(!lua->load_file(script)) {
        return false;
    }

    return register_stage_node_from_lua_state(lua->lua_state(), class_name);
}

bool StageNodeManager::register_stage_node_from_lua_state(lua_State* L,
                                                          const char* class_name) {
    luabridge::LuaRef klass = luabridge::getGlobal(L, class_name);
    if(klass.isNil()) {
        S_ERROR("Unable to find specified class in Lua file: {0}", class_name);
        return false;
    }

    luabridge::LuaRef meta = klass["Meta"];
    if(meta.isNil()) {
        S_ERROR("Lua class '{0}' is missing Meta table", class_name);
        return false;
    }

    auto node_type_val = meta["node_type"];
    if(node_type_val.isNil()) {
        S_ERROR("Lua class '{0}' Meta table is missing node_type", class_name);
        return false;
    }

    // Read the node_type from Lua using the raw Lua API to avoid LuaBridge
    // cast issues on 32-bit platforms (e.g. Dreamcast).  The value was stored
    // as a uint32_t C++ integer, which Lua holds as a double (lua_Number).
    // We push the table and the key onto the stack, get the value, and convert
    // it directly.
    meta.push();
    lua_pushstring(L, "node_type");
    lua_gettable(L, -2);
    StageNodeType node_id = static_cast<StageNodeType>(lua_tonumber(L, -1));
    lua_pop(L, 2);

    if(node_id == 0) {
        S_ERROR("Lua class '{0}' has invalid node_type (got 0)", class_name);
        return false;
    }

    std::string name = meta["name"];

    // Capture the raw lua_State* and the class name string rather than a
    // luabridge::LuaRef.  LuaRef holds a Lua registry reference and calls
    // luaL_unref in its destructor; if the lambda is destroyed after
    // lua_close() (which happens when StageNodeManager::~StageNodeManager
    // runs after the interpreter has been torn down), that luaL_unref call
    // writes into already-freed Lua memory and corrupts the heap.
    // A raw pointer drop and a std::string drop are both no-ops, so
    // destroying the lambda after lua_close() is safe.
    std::string klass_name(class_name);

    // Parse klass.params if defined — a table mapping param names to
    // descriptor tables created with smlt.define_node_param().
    std::set<NodeParam> lua_params;
    luabridge::LuaRef params_table = klass["params"];
    if(params_table.isTable()) {
        int order = 0;
        for(auto [key, val] : luabridge::pairs(params_table)) {
            if(!key.isString() || !val.isTable()) {
                continue;
            }

            auto marker = val["__is_node_param"];
            if(!marker.isBool() || !marker.cast<bool>().valueOr(false)) {
                continue;
            }

            std::string pname = key.cast<std::string>().valueOr("");
            if(pname.empty()) continue;

            int type_int = val["param_type"].cast<int>().valueOr(
                static_cast<int>(NODE_PARAM_TYPE_INVALID));
            NodeParamType param_type = static_cast<NodeParamType>(type_int);
            if(param_type == NODE_PARAM_TYPE_INVALID) continue;

            std::string desc = val["description"].cast<std::string>().valueOr("");
            bool required = val["required"].cast<bool>().valueOr(true);

            // Convert Lua default_value to a C++ ParamValue based on param_type.
            optional<ParamValue> default_val;
            auto lua_default = val["default_value"];
            if(!lua_default.isNil()) {
                switch(param_type) {
                    case NODE_PARAM_TYPE_FLOAT:
                        default_val = ParamValue(lua_default.cast<float>().valueOr(0.0f));
                        break;
                    case NODE_PARAM_TYPE_INT:
                        default_val = ParamValue(
                            (int)lua_default.cast<int>().valueOr(0));
                        break;
                    case NODE_PARAM_TYPE_BOOL:
                        default_val = ParamValue(
                            lua_default.cast<bool>().valueOr(false));
                        break;
                    case NODE_PARAM_TYPE_STRING:
                        default_val = ParamValue(
                            lua_default.cast<std::string>().valueOr(""));
                        break;
                    case NODE_PARAM_TYPE_FLOAT_ARRAY:
                        if(lua_default.isTable()) {
                            FloatArray arr;
                            for(int i = 1; ; ++i) {
                                auto item = lua_default[i];
                                if(item.isNil()) break;
                                arr.push_back(item.cast<float>().valueOr(0.0f));
                            }
                            default_val = ParamValue(arr);
                        }
                        break;
                    default:
                        break;
                }
            }

            lua_params.insert(
                NodeParam(order++, pname, param_type, default_val, desc, required));
        }
    }

    return register_stage_node(node_id, name.c_str(), sizeof(LuaStageNode),
                               alignof(LuaStageNode),
                               [L, klass_name, node_id, name, lua_params, this](void* mem) -> StageNode* {
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
            // cls:new() returns a plain Lua wrapper table; _cpp_node is not
            // set yet — that happens below once placement-new succeeds.
            luabridge::LuaRef instance =
                *constructor.call<luabridge::LuaRef>(klass, scene_);

            // Create the one true LuaStageNode that lives in the scene tree.
            // Pass all params directly so the constructor no longer needs to
            // read them back through the (not-yet-linked) Lua table.
            LuaStageNode* node = new(mem) LuaStageNode(
                scene_, node_id, name, lua_params, instance);

            // Wire the wrapper table to the real C++ node using a raw table
            // set so that __newindex metamethods (if any) are bypassed.
            instance.rawsetField("_cpp_node", node);

            return node;
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
