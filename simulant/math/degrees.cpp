#include "degrees.h"
#include "radians.h"
#include "utils.h"

namespace smlt {

Degrees::Degrees(const Radians &rhs):
    value(rhs.value * PI_UNDER_180) {

}

Degrees lerp_angle(Degrees a, Degrees b, float t) {
    return Degrees(lerp_angle(Radians(a), Radians(b), t));
}

Degrees to_degrees(const Radians& radians) {
    return Degrees(radians.value * PI_UNDER_180);
}

}
