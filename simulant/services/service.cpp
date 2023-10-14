#include "service.h"

namespace smlt {

void Service::update(float dt) {
    Updateable::update(dt);
}

void Service::late_update(float dt) {
    Updateable::late_update(dt);
}

void Service::fixed_update(float step) {
    Updateable::fixed_update(step);
}



}
