#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "generic/managed.h"
#include "kazbase/unicode.h"
#include "kazbase/logging.h"

#include <lua.hpp>
#include <luabind/luabind.hpp>

namespace kglt {

enum LuaResult {
    LUA_RESULT_EOF,
    LUA_RESULT_SUCCESS,
    LUA_RESULT_ERROR
};

template<typename T>
class LuaClass {
public:
    static void export_to(lua_State& state) {
        T::do_lua_export(state);
    }
};

/*
 * USAGE:
 *
 * Interpreter::ptr i = Interpreter::create();
 * i.register_class<Scene>();
 * i.register_class<SubScene>();
 */
class Interpreter:
    public Managed<Interpreter> {

public:
    Interpreter():
        state_(nullptr) {

        L_INFO("Initializing LUA interpreter");
        state_ = luaL_newstate();
        luabind::open(state_);        
        luaopen_base(state_);
    }

    ~Interpreter() {
        L_INFO("Shutting down LUA interpreter");
        lua_close(state_);
    }

    void run_file(const std::string& filename);
    LuaResult run_string(const std::string& statement, unicode &output);

    void update();

    template<typename T>
    void register_class() {
        L_INFO(_u("Registering class '{0}' with lua...").format(typeid(T).name()));
        T::export_to(*state_);
    }

private:
    lua_State* state_;
};

}

#endif // INTERPRETER_H
