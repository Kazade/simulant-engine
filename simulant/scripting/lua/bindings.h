#pragma once

// clang-format off
#include "minilua.h"
#include "LuaBridge.h"
// clang-format on

namespace smlt {
void lua_bind(lua_State* state);
}
