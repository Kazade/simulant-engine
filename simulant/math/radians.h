#pragma once

namespace smlt {

struct Degrees;

class Radians {
private:
    float value_ = 0.0f;

public:
    Radians():
        value_(0) {}

    explicit Radians(float value):
        value_(value) {}

    Radians(const Degrees& rhs);

    float to_float() const {
        return value_;
    }

    Degrees to_degrees() const;

    Radians operator*(float scalar) {
        return Radians(to_float() * scalar);
    }

    Radians& operator*=(float scalar) {
        value_ *= scalar;
        return *this;
    }
};

Radians lerp_angle(Radians a, Radians b, float t);

typedef Radians Rad;
}
