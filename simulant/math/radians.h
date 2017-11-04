#pragma once

namespace smlt {

struct Degrees;

struct Radians {
    Radians():
        value(0) {}

    explicit Radians(float value):
        value(value) {}

    Radians(const Degrees& rhs);

    float value;
};


Radians to_radians(const Degrees& degrees);
Radians lerp_angle(Radians a, Radians b, float t);

}
