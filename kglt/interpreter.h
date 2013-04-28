#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "generic/managed.h"

#include <lua.hpp>
#include <luabind/luabind.hpp>

namespace kglt {

template<typename T>
class LuaClass {
public:
    static void export_to(lua_State& state) {
        typename T::do_lua_export(state);
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

        state_ = luaL_newstate();
        luabind::open(state_);
    }

    ~Interpreter() {
        lua_close(state_);
    }

    void queue_string(const unicode& str);
    void update();

    template<typename T>
    void register_class() {
        typename T::export_to(*state_);
    }

private:
    lua_State* state_;
};

}

#endif // INTERPRETER_H
