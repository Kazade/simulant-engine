#pragma once

#include <cmath>
#include "../utils/unicode.h"
#include "../utils/formatter.h"
#include "utils.h"

namespace smlt {

struct Vec3;
struct Vec4;
struct Degrees;

struct Vec2 {

public:
    static const Vec2 NEGATIVE_X;
    static const Vec2 POSITIVE_X;
    static const Vec2 NEGATIVE_Y;
    static const Vec2 POSITIVE_Y;

    static const Vec2 DOWN;
    static const Vec2 LEFT;
    static const Vec2 ONE;
    static const Vec2 RIGHT;
    static const Vec2 UP;
    static const Vec2 ZERO;

    float x;
    float y;

    Vec2():
        x(0.0f), y(0.0f) {
    }

    Vec2(float x, float y):
        x(x), y(y) {

    }

    Vec2 rotated_by(const Degrees& degrees) const;

    float length() const {
        return fast_sqrt(x * x + y * y);
    }

    float length_squared() const {
        return x * x + y * y;
    }

    void normalize() {
        float l = fsrra(length_squared());
        x *= l;
        y *= l;
    }

    Vec2 normalized() const {
        Vec2 ret = *this;
        ret.normalize();
        return ret;
    }

    void limit(float l) {
        if(length() > l) {
            normalize();
            *this *= l;
        }
    }

    Vec2 operator*(float rhs) const {
        Vec2 result(x * rhs, y * rhs);
        return result;
    }

    Vec2& operator*=(float rhs) {
        *this = *this * rhs;
        return *this;
    }

    Vec2& operator+=(const Vec2& rhs) {
        *this = *this + rhs;
        return *this;
    }

    Vec2& operator-=(const Vec2& rhs) {
        *this = *this - rhs;
        return *this;
    }

    Vec2 operator+(const Vec2& rhs) const {
        return Vec2(x + rhs.x, y + rhs.y);
    }

    Vec2& operator/=(float rhs) {
        x = fast_divide(x, rhs);
        y = fast_divide(y, rhs);
        return *this;
    }

    Vec2 operator/(float rhs) const {
        Vec2 result(fast_divide(x, rhs), fast_divide(y, rhs));
        return result;
    }

    Vec2 operator-() const {
        return Vec2(-x, -y);
    }

    Vec2 operator-(const Vec2& rhs) const {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    float dot(const Vec2& rhs) const {
        return x * rhs.x + y * rhs.y;
    }

    Vec3 xyz(float z = 0.0f) const;

    Vec4 xyzw(float z=0.0f, float w=1.0f) const;

    friend std::ostream& operator<<(std::ostream& stream, const Vec2& vec);

    bool equals(const Vec2& rhs) const {
        return x == rhs.x && y == rhs.y;
    }

    friend bool operator==(const Vec2& lhs, const Vec2& rhs);
};

bool operator==(const Vec2& lhs, const Vec2& rhs);
bool operator!=(const Vec2& lhs, const Vec2& rhs);
Vec2 operator*(float lhs, const Vec2& rhs);

std::ostream& operator<<(std::ostream& stream, const Vec2& vec);

}
