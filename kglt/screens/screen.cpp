#include "screen.h"
#include "window_base.h"

namespace kglt {

Screen::Screen(WindowBase &window):
    window_(window) {

}

Screen::~Screen() {
    try {
        unload();
    } catch(...) {}
}

void Screen::load() {
    do_load();
    is_loaded_ = true;
}

void Screen::unload() {
    do_unload();
    is_loaded_ = false;
}

void Screen::activate() {
    do_activate();
}

void Screen::deactivate() {
    do_deactivate();
}

void Screen::update(double dt) {
    do_update(dt);
}

}

