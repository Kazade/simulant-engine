#ifndef API_H
#define API_H

struct lua_State;

namespace kglt {

void export_lua_api(lua_State* state);

}

#endif // API_H
