
#include "interpreter.h"

bool smlt::LuaInterpreter::load_string(const char* data) {
    int result = luaL_loadstring(state_, data);
    if(result != LUA_OK) {
        S_ERROR("Unable to load lua script: {0}", lua_tostring(state_, -1));
        return false;
    }

    if(lua_pcall(state_, 0, 0, 0) != LUA_OK) {
        S_ERROR("Unable to run lua script: {0}", lua_tostring(state_, -1));
        return false;
    }

    return true;
}

bool smlt::LuaInterpreter::load_file(const Path& script) {
    int result = luaL_loadfilex(state_, script.str().c_str(), NULL);
    if(result != LUA_OK) {
        S_ERROR("Unable to load lua script");
        return false;
    }

    if(lua_pcall(state_, 0, 0, 0) != LUA_OK) {
        S_ERROR("Unable to run lua script");
        return false;
    }

    return true;
}

smlt::optional<luabridge::LuaRef>
    smlt::LuaInterpreter::get_global(const char* name) {
    luabridge::LuaRef ref = luabridge::getGlobal(state_, name);
    if(ref.isNil()) {
        return no_value;
    }

    return ref;
}

// ---------------------------------------------------------------------------
// smlt.define_node(name) — Lua helper that creates a StageNode subclass.
//
// Usage in a Lua script:
//
//   MyNode = smlt.define_node("my_node")
//
//   function MyNode:on_update(dt)
//       print("tick", dt)
//   end
//
// The helper sets up:
//   - the class table with __index pointing to itself
//   - the Meta table (node_type hash + name) via smlt.stage_node_meta()
//   - a default :new(scene) constructor that returns a plain Lua wrapper
//     table; the C++ factory fills in a _cpp_node field (via rawset, so
//     metamethods are bypassed) pointing to the real LuaStageNode once
//     placement-new succeeds.
//   - an __index metamethod on each instance that checks for Lua-defined
//     methods first, then forwards unknown keys to _cpp_node so that C++
//     properties like transform, scene, and node_type are transparently
//     accessible as self.transform etc.
// ---------------------------------------------------------------------------
static const char* k_define_node_helper = R"lua(
rawset(smlt, "define_node", function(name)
    local cls = {}
    cls.__index = cls
    cls.Meta = smlt.stage_node_meta(name)

    -- Default constructor.  Called by the C++ registration machinery as
    -- cls:new(scene), i.e. constructor(cls, scene_).
    -- Returns a plain Lua wrapper table.  C++ will rawset _cpp_node on it
    -- (bypassing __newindex) once the real LuaStageNode has been created.
    function cls:new(scene)
        local wrapper = {}
        setmetatable(wrapper, {
            __index = function(t, k)
                -- Lua-defined methods (e.g. on_update) take priority.
                local v = cls[k]
                if v ~= nil then return v end
                -- Forward to the real C++ node stored by the factory.
                local cpp = rawget(t, "_cpp_node")
                if cpp ~= nil then return cpp[k] end
                return nil
            end
        })
        return wrapper
    end

    return cls
end)

rawset(smlt, "define_node_param", function(param_type, description, default_value)
    return {
        __is_node_param = true,
        param_type = param_type,
        description = description or "",
        default_value = default_value,
        required = (default_value == nil)
    }
end)

-- Helper to create a child node from a Lua script.
-- Usage: local child = smlt.create_child_node(self, "stage", {param1 = value1})
rawset(smlt, "create_child_node", function(parent_node, type_name, params)
    if not parent_node or not parent_node._cpp_node then
        return nil
    end
    local ok, result = pcall(function()
        return parent_node._cpp_node:create_child(type_name, params or {})
    end)
    if not ok then
        print("smlt.create_child_node error: " .. tostring(result))
        return nil
    end
    return result
end)

-- Helper to create a mixin from a Lua script.
-- Usage: local mixin = smlt.create_mixin(self, "stage", {param1 = value1})
rawset(smlt, "create_mixin", function(host_node, type_name, params)
    if not host_node or not host_node._cpp_node then
        return nil
    end
    local ok, result = pcall(function()
        return host_node._cpp_node:create_mixin(type_name, params or {})
    end)
    if not ok then
        print("smlt.create_mixin error: " .. tostring(result))
        return nil
    end
    return result
end)
)lua";

bool smlt::LuaInterpreter::on_init() {
    state_ = lua_newstate(LuaInterpreter::l_alloc, NULL, 47);
    luaL_openlibs(state_);

    lua_bind(state_);

    // Inject the smlt.define_node() helper after the C++ bindings are in place
    // (it depends on smlt.StageNode and smlt.stage_node_meta being available).
    if(!load_string(k_define_node_helper)) {
        S_ERROR("Failed to load smlt.define_node Lua helper");
        return false;
    }

    return true;
}
