#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "generic/managed.h"
#include "kazbase/unicode/unicode.h"
#include "kazbase/logging/logging.h"

#include <lua.hpp>
#include <luabind/luabind.hpp>

namespace kglt {

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
    }

    ~Interpreter() {
        L_INFO("Shutting down LUA interpreter");
        lua_close(state_);
    }

    template<typename Result>
    Result call_function() {

    }

    template<typename Result, typename A1>
    Result call_function(A1 arg1) {

    }

    void run_file(const std::string& filename);
    void queue_string(const unicode& str);
    void update();

    template<typename T>
    void register_class() {
        L_INFO(unicode("Registering class '{0}' with lua...").format(demangle(typeid(T).name()).encode()).encode());
        T::export_to(*state_);
    }

private:
    lua_State* state_;
};

}

#endif // INTERPRETER_H
