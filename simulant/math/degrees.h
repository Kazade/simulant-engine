#pragma once

#if defined(_MSC_VER)
#include "radians.h"
#endif

namespace smlt {

#if !defined(_MSC_VER)
struct Radians;
#endif

struct Degrees {
private:
    float value_ = 0.0f;

public:
    Degrees():
        value_(0) {}

    explicit Degrees(float value):
        value_(value) {}

    Degrees(const Radians& rhs);

    float to_float() const {
        return value_;
    }

    Radians to_radians() const;

    Degrees operator*(float scalar) {
        return Degrees(to_float() * scalar);
    }

    Degrees& operator*=(float scalar) {
        value_ *= scalar;
        return *this;
    }

    bool operator<(const Degrees& d) {
        return value_ < d.value_;
    }

    bool operator<=(const Degrees& d) {
        return value_ <= d.value_;
    }

    bool operator>(const Degrees& d) {
        return value_ > d.value_;
    }

    bool operator>=(const Degrees& d) {
        return value_ >= d.value_;
    }

    Degrees operator-() const {
        Degrees ret = *this;
        ret.value_ = -ret.value_;
        return ret;
    }

    Degrees operator-=(const Degrees& rhs) const {
        Degrees ret = *this;
        ret.value_ -= rhs.value_;
        return ret;
    }

    Degrees operator+=(const Degrees& rhs) const {
        Degrees ret = *this;
        ret.value_ += rhs.value_;
        return ret;
    }

    bool operator==(const Degrees& rhs) const {
        return value_ == rhs.value_;
    }

    bool operator!=(const Degrees& rhs) const {
        return !(*this == rhs);
    }

    bool is_effectively_equal_to(const Degrees& rhs, float epsilon=0.0f) const {
        // Returns equal if the values represent basically the same thing (e.g. -90 == 270)
        float rhs_v = rhs.value_;
        if(rhs_v < 0) rhs_v += 360.0f;

        float lhs_v = value_;
        if(lhs_v < 0) lhs_v += 360.0f;

        return lhs_v - epsilon < rhs_v && lhs_v + epsilon > rhs_v;
    }
};

Degrees lerp_angle(Degrees a, Degrees b, float t);

typedef Degrees Deg;

}
