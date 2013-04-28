#include "interpreter.h"

namespace kglt {

void Interpreter::run_file(const std::string& filename) {
    luaL_dofile(this->state_, filename.c_str());
}



}
