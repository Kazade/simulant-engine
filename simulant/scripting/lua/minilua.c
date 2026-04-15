// Disable jump table implementation for LTO compatibility
// The computed goto dispatch mechanism in the Lua VM creates local labels (.L*)
// that are not properly handled by LTO (Link Time Optimization). Disabling
// LUA_USE_JUMPTABLE forces the use of switch-case dispatch instead, which is
// LTO-compatible and only has negligible performance impact.
#define LUA_USE_JUMPTABLE 0

#define LUA_IMPL

#ifdef __DREAMCAST__
#define LUA_32BITS 1
#endif

#include "minilua.h"