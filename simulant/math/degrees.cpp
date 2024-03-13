#include "degrees.h"
#include "radians.h"
#include "utils.h"

namespace smlt {

Degrees::Degrees(const Radians &rhs):
    value_(rhs.to_float() * PI_UNDER_180) {

}

Radians Degrees::to_radians() const {
    return Radians(*this);
}

Degrees lerp_angle(Degrees a, Degrees b, float t) {
    return Degrees(lerp_angle(Radians(a), Radians(b), t));
}

}
