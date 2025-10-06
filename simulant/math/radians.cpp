#include "radians.h"
#include "degrees.h"
#include "utils.h"

smlt::Radians operator""_rad(long double v) {
    return smlt::Radians(float(v));
}

namespace smlt {

Radians::Radians(const Degrees &rhs):
    value_(rhs.to_float() * PI_OVER_180) {
}

Degrees Radians::to_degrees() const {
    return Degrees(*this);
}

}
