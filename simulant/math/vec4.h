#pragma once

#include <cmath>
#include <ostream>
#include "utils.h"

namespace smlt {

struct Vec2;
struct Vec3;

struct Vec4 {
    friend struct Vec3;
    friend struct Mat4;

    float x;
    float y;
    float z;
    float w;

    Vec4():
        x(0), y(0), z(0), w(0) {
    }

    Vec4(float x, float y, float z, float w):
        x(x), y(y), z(z), w(w) {

    }

    Vec4(const Vec3& v, float w);

    bool equals(const Vec4& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

    bool operator==(const Vec4& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

    bool operator!=(const Vec4& rhs) const {
        return (x != rhs.x) || (y != rhs.y) || (z != rhs.z) || (w != rhs.w);
    }

    Vec4 operator-(const Vec4& rhs) const {
        return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }

    Vec4 operator*(const float& rhs) const {
        return Vec4(
            x * rhs,
            y * rhs,
            z * rhs,
            w * rhs
        );
    }

    float length() const {
        return fast_sqrt(length_squared());
    }

    float length_squared() const {
        return fast_sum_of_squares(x, y, z, w);
    }

    void normalize() {
        float l = fsrra(length_squared());
        x *= l;
        y *= l;
        z *= l;
        w *= l;
    }

    const smlt::Vec4 normalized() const {
        smlt::Vec4 result = *this;
        result.normalize();
        return result;
    }

    Vec4 operator+(const Vec4& rhs) const {
        return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }

    Vec3 xyz() const;
    Vec2 xy() const;
};

std::ostream& operator<<(std::ostream& stream, const Vec4& vec);


}
