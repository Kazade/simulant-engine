#include "degrees.h"
#include "radians.h"
#include "utils.h"

smlt::Degrees operator""_deg(long double v) {
    return smlt::Degrees(float(v));
}

namespace smlt {

Degrees::Degrees(const Radians &rhs):
    value_(rhs.to_float() * PI_UNDER_180) {

}

Radians Degrees::to_radians() const {
    return Radians(*this);
}


}
