#include "radians.h"
#include "degrees.h"
#include "utils.h"

namespace smlt {

Radians::Radians(const Degrees &rhs):
    value(rhs.value * PI_OVER_180) {
}

Radians to_radians(const Degrees& degrees) {
    return Radians(degrees.value * PI_OVER_180);
}

Radians lerp_angle(Radians a, Radians b, float t) {
    float from_angle = std::fmod(a.value + TWO_PI, TWO_PI);
    float to_angle = std::fmod(b.value + TWO_PI, TWO_PI);

    auto diff = std::fabs(from_angle - to_angle);

    if(diff < PI) {
        return Radians(lerp(from_angle, to_angle, t));
    } else {
        if(from_angle > to_angle) {
            from_angle = from_angle - TWO_PI;
            return Radians(lerp(from_angle, to_angle, t));
        } else {
            to_angle = to_angle - TWO_PI;
            return Radians(lerp(from_angle, to_angle, t));
        }
    }
}

}
