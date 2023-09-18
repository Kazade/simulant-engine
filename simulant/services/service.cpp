#include "service.h"

namespace smlt {

void Service::update(float dt) {
    on_update(dt);
}

void Service::late_update(float dt) {
    on_late_update(dt);
}

void Service::fixed_update(float step) {
    on_fixed_update(step);
}



}
