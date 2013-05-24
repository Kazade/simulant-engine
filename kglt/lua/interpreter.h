#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../generic/managed.h"
#include "../kazbase/unicode.h"
#include "../kazbase/logging.h"
#include "../types.h"
#include "types.h"

#include <lua.hpp>

#include "luabind/luabind.hpp"
#include "luabind/class_info.hpp"

namespace kglt {

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
    Interpreter();
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

    template<typename T>
    void add_global(const std::string& name, T& what) {
        luabind::globals(state_)[name.c_str()] = &what;
    }

    lua_State* state() { return state_; }

private:
    lua_State* state_;
};

}

#endif // INTERPRETER_H
