
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
//   - a default :new(scene) constructor that:
//       * constructs the C++ LuaStageNode shell (smlt.StageNode)
//       * wraps it in a plain Lua table so setmetatable() works (Lua 5.5
//         forbids calling setmetatable on userdata)
//       * captures the shell as a closure upvalue rather than storing it as
//         a visible field, so C++ properties (transform, scene, node_type,
//         …) are accessible directly as self.transform etc. with no _cpp
//         prefix needed
//       * installs an __index metamethod that checks for Lua-defined methods
//         first, then transparently falls back to the C++ shell upvalue
// ---------------------------------------------------------------------------
static const char* k_define_node_helper = R"lua(
rawset(smlt, "define_node", function(name)
    local cls = {}
    cls.__index = cls
    cls.Meta = smlt.stage_node_meta(name)

    -- Default constructor.  Called by the C++ registration machinery as
    -- cls:new(scene), i.e. constructor(cls, scene_).
    function cls:new(scene)
        -- Build the C++ shell via the LuaBridge-registered constructor.
        -- Dot-syntax is required here; colon-syntax would pass `self` as the
        -- first argument and confuse LuaBridge's overload resolution.
        local cpp = smlt.StageNode(
            scene,
            cls.Meta.node_type,
            cls.Meta.name,
            {}
        )

        -- cpp is intentionally NOT stored as a table field.  It lives only
        -- as a closure upvalue so that it is invisible to Lua code.  This
        -- means C++ properties (transform, scene, node_type, …) are
        -- accessible directly as self.transform / self.scene / etc. without
        -- any ._cpp prefix, because __index below delegates to cpp for any
        -- key that is not a Lua-defined method.
        local wrapper = {}
        setmetatable(wrapper, {
            __index = function(t, k)
                -- Lua-defined methods (e.g. on_update) take priority.
                local v = cls[k]
                if v ~= nil then return v end
                -- Transparently fall back to the LuaBridge-bound C++ shell.
                return cpp[k]
            end
        })
        return wrapper
    end

    return cls
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
