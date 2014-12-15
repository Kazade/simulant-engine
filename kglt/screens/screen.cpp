#include "screen.h"
#include "../window_base.h"

namespace kglt {

ScreenBase::ScreenBase(WindowBase &window):
    window_(window) {

}

ScreenBase::~ScreenBase() {

}

void ScreenBase::load() {
    do_load();
    is_loaded_ = true;
}

void ScreenBase::unload() {
    do_unload();
    is_loaded_ = false;
}

void ScreenBase::activate() {
    do_activate();
}

void ScreenBase::deactivate() {
    do_deactivate();
}

void ScreenBase::update(double dt) {
    do_update(dt);
}

}

