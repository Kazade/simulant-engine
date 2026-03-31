#pragma once

#include "../interpreter.h"
#include "minilua.h"

namespace smlt {

class LuaInterpreter: public Interpreter {
public:
    std::string get_string(const std::string& lookup);

private:
    bool on_init() override {
        state_ = lua_newstate(LuaInterpreter::l_alloc, NULL, 47);
        luaL_openlibs(state_);
        return true;
    }

    void on_update(float dt) override {}

    void on_clean_up() override {
        lua_close(state_);
    }

    lua_State* state_ = nullptr;

    static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
        (void)ud;
        (void)osize; /* not used */
        if(nsize == 0) {
            free(ptr);
            return NULL;
        } else {
            return realloc(ptr, nsize);
        }
    }
};

} // namespace smlt
