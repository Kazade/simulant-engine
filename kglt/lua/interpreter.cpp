#include "interpreter.h"

namespace kglt {

Interpreter::Interpreter():
    state_(nullptr) {

    L_INFO("Initializing LUA interpreter");
    state_ = luaL_newstate();
    luaL_openlibs(state_);
    luabind::open(state_);
    luabind::bind_class_info(state_);

    std::string dir_command = R"x(
        function dir(obj)
            output = {}

            for k, v in pairs(class_info(obj).methods) do
                table.insert(output, k .. "()")
            end

            for k, v in pairs(class_info(obj).attributes) do
                table.insert(output, v)
            end

            table.sort(output)

            print(table.concat(output, ", "))
        end
    )x";

    unicode output;
    run_string(dir_command, output);
}

void Interpreter::run_file(const std::string& filename) {
    luaL_dofile(this->state_, filename.c_str());
}

LuaResult Interpreter::run_string(const std::string& statement, unicode& output) {
    int stack_size = lua_gettop(this->state_);

    std::string final = "return " + statement;
    int result = luaL_loadbuffer(this->state_, final.c_str(), final.size(), "console");
    if(result == LUA_ERRSYNTAX) {
        //Try without the return
        result = luaL_loadbuffer(this->state_, statement.c_str(), statement.size(), "console");
    }

    if(result != 0) {
        size_t len;
        output = lua_tolstring(this->state_, -1, &len);
        lua_pop(this->state_, 1);

        if(result == LUA_ERRSYNTAX && output.contains("<eof>")) {
            return LUA_RESULT_EOF;
        } else {
            return LUA_RESULT_ERROR;
        }
    }

    int top = lua_gettop(this->state_) - 1;
    result = lua_pcall(this->state_, 0, LUA_MULTRET, 0);

    if(result) {
        //If there was an error, read the error message from the stack
        output = luaL_checkstring(this->state_, -1);
        lua_pop(this->state_, 1);
        return LUA_RESULT_ERROR;
    }

    std::vector<std::string> results;

    int n = lua_gettop(this->state_) - top;//Get the number of results

    if(n) {
        lua_getglobal(this->state_, "tostring");
        for(int i = 1; i <= n; ++i) {
            lua_pushvalue(this->state_, -1);
            lua_pushvalue(this->state_, top + i);

            result = lua_pcall(this->state_, 1, 1, 0);
            size_t len = 0;
            const char *s = 0;
            if (result == 0) {
                s = lua_tolstring(this->state_, -1, &len);
                results.push_back(s);
            }
            lua_pop(this->state_, 1);
        }

        output = _u("    ").join(results).encode();
    }

    //Pop everything off the stack so we are back in the original place
    while(lua_gettop(this->state_) > stack_size) {
        lua_pop(this->state_, 1);
    }


    return LUA_RESULT_SUCCESS;

}


}
