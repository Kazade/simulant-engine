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

    Vec4 operator+(const Vec4& rhs) const {
        return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
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
#ifdef __DREAMCAST__
        return MATH_Sum_of_Squares(x, y, z, w);
#else
        return dot(*this);
#endif
    }

    void normalize() {
        float l = fast_inverse_sqrt(length_squared());
        x *= l;
        y *= l;
        z *= l;
        w *= l;
    }

    float dot(const Vec4& rhs) const {
#ifdef __DREAMCAST__
        return MATH_fipr(x, y, z, w, rhs.x, rhs.y, rhs.z, rhs.w);
#else
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
#endif
    }

    const smlt::Vec4 normalized() const {
        smlt::Vec4 result = *this;
        result.normalize();
        return result;
    }

    Vec3 xyz() const;
    Vec2 xy() const;
};

std::ostream& operator<<(std::ostream& stream, const Vec4& vec);


}
