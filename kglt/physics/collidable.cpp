#include "collidable.h"
#include "../object.h"

namespace kglt {

Collidable::Collidable(Object* owner):
    owner_(owner) {

    assert(owner);

    owner->signal_made_responsive().connect([=]() {
        attach_to_responsive_body(owner->responsive_body());
    });
}

}
