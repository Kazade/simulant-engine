#pragma once

#include "interpreter.h"

#include <functional>

namespace smlt {

void lua_bind(lua_State* state);

/* A hook invoked with the interpreter's lua_State immediately after the
 * built-in smlt.* bindings are installed. Lets engine extensions and
 * applications register Lua bindings for their own C++ types (e.g. a custom
 * pathfinding stage node), which the engine's bindings.cpp can't see.
 *
 * Call register_lua_binding_hook() before the Lua interpreter is created
 * (i.e. before the first register_scene()/ensure_lua_ready()). */
typedef std::function<void(lua_State*)> LuaBindingHook;

void register_lua_binding_hook(LuaBindingHook hook);

} // namespace smlt
