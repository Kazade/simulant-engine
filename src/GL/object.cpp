#include "object.h"

namespace GL {

Object::~Object() {
    if(has_parent()) {
        parent().detach_child(this);
    }
}

void Object::move(float x, float y, float z) {
    position_.x = x;
    position_.y = y;
    position_.z = z;
}

}
