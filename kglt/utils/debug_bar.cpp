
#include "debug_bar.h"

namespace kglt {

DebugBar::DebugBar(int32_t window_width, int32_t window_height):
    visible_(false) {
    TwInit(TW_OPENGL, NULL);
    ant_bar_ = TwNewBar("Debug Bar");
    TwWindowSize(window_width, window_height);
}

void DebugBar::add_read_only_variable(const unicode& id, bool* variable) {
    TwAddVarRO(ant_bar_, id.encode().c_str(), TW_TYPE_BOOLCPP, variable, nullptr);
}

void DebugBar::add_read_only_variable(const unicode& id, int32_t* variable) {
    TwAddVarRO(ant_bar_, id.encode().c_str(), TW_TYPE_INT32, variable, nullptr);
}

void DebugBar::add_read_only_variable(const unicode& id, float* variable) {
    TwAddVarRO(ant_bar_, id.encode().c_str(), TW_TYPE_FLOAT, variable, nullptr);
}

void DebugBar::add_read_only_variable(const unicode& id, double* variable) {
    TwAddVarRO(ant_bar_, id.encode().c_str(), TW_TYPE_DOUBLE, variable, nullptr);
}

void DebugBar::add_read_only_variable(const unicode &id, kmVec3 *variable) {
    TwAddVarRO(ant_bar_, id.encode().c_str(), TW_TYPE_DIR3F, variable, nullptr);
}

void DebugBar::add_read_only_variable(const unicode &id, kmQuaternion *variable) {
    TwAddVarRO(ant_bar_, id.encode().c_str(), TW_TYPE_QUAT4F, variable, nullptr);
}

bool DebugBar::handle_event(SDL_Event& event) {
    return TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
}

void DebugBar::render() {
    if(!visible_) return;

    TwDraw();
}

DebugBar::~DebugBar() {
    TwTerminate();
}

}
