#pragma once

namespace smlt {

struct Radians;

struct Degrees {
    Degrees():
        value(0) {}

    explicit Degrees(float value):
        value(value) {}

    Degrees(const Radians& rhs);

    float value;

    Degrees operator-() const {
        Degrees ret = *this;
        ret.value = -ret.value;
        return ret;
    }

    Degrees operator-=(const Degrees& rhs) const {
        Degrees ret = *this;
        ret.value -= rhs.value;
        return ret;
    }

    Degrees operator+=(const Degrees& rhs) const {
        Degrees ret = *this;
        ret.value += rhs.value;
        return ret;
    }

    bool operator==(const Degrees& rhs) const {
        return value == rhs.value;
    }

    bool operator!=(const Degrees& rhs) const {
        return !(*this == rhs);
    }

    bool is_effectively_equal_to(const Degrees& rhs, float epsilon=0.0f) {
        // Returns equal if the values represent basically the same thing (e.g. -90 == 270)
        float rhs_v = rhs.value;
        if(rhs_v < 0) rhs_v += 360.0f;

        float lhs_v = value;
        if(lhs_v < 0) lhs_v += 360.0f;

        return lhs_v - epsilon < rhs_v && lhs_v + epsilon > rhs_v;
    }
};

Degrees to_degrees(const Radians& radians);
Degrees lerp_angle(Degrees a, Degrees b, float t);

}
