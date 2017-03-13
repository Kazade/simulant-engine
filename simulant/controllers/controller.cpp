#include "controller.h"

namespace smlt {

void Controller::enable() {
    is_enabled_ = true;
}

void Controller::disable() {
    is_enabled_ = false;
}

void Controller::_update_thunk(float dt) {
    if(!is_enabled_) {
        return;
    }

    Updateable::_update_thunk(dt);
}

void Controller::_late_update_thunk(float dt) {
    if(!is_enabled_) {
        return;
    }

    Updateable::_late_update_thunk(dt);
}

void Controller::_fixed_update_thunk(float step) {
    if(!is_enabled_) {
        return;
    }

    Updateable::_fixed_update_thunk(step);
}

}
