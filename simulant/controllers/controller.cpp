#include "controller.h"

namespace smlt {

void Controller::enable() {
    is_enabled_ = true;
}

void Controller::disable() {
    is_enabled_ = false;
}

void Controller::update(double dt) {
    if(!is_enabled_) {
        return;
    }

    do_update(dt);
}

void Controller::late_update(double dt) {
    if(!is_enabled_) {
        return;
    }

    do_late_update(dt);
}

void Controller::fixed_update(double step) {
    if(!is_enabled_) {
        return;
    }

    do_fixed_update(step);
}

}
