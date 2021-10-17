#pragma once

#include "half.hpp"
#include "../math/vec3.h"
#include "../math/vec2.h"

namespace smlt {

using half = half_float::half;

struct HalfVec3 {
    half x, y, z;

    HalfVec3() = default;
    HalfVec3(float x, float y, float z):
        x(x), y(y), z(z) {}

    HalfVec3 operator+(const HalfVec3& rhs) const {
        return HalfVec3{x + rhs.x, y + rhs.y, z + rhs.z};
    }

    operator Vec3() const {
        return Vec3(x, y, z);
    }
};


struct HalfVec2 {
    half x, y;

    HalfVec2() = default;
    HalfVec2(float x, float y):
        x(x), y(y) {}

    HalfVec2 operator+(const HalfVec3& rhs) const {
        return HalfVec2{x + rhs.x, y + rhs.y};
    }

    operator Vec2() const {
        return Vec2(x, y);
    }
};

}
