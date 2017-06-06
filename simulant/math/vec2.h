#pragma once

#include <cmath>
#include "../utils/unicode.h"

namespace smlt {

struct Degrees;

struct Vec2 {
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
        return sqrtf(x * x + y * y);
    }

    void normalize() {
        float l = 1.0f / length();
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
        *this = *this / rhs;
        return *this;
    }

    Vec2 operator/(float rhs) const {
        Vec2 result(x / rhs, y / rhs);
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

    unicode to_string() const {
        return _u("({0},{1})").format(x, y);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vec2& vec);
    friend bool operator==(const Vec2& lhs, const Vec2& rhs);
};

bool operator==(const Vec2& lhs, const Vec2& rhs);
bool operator!=(const Vec2& lhs, const Vec2& rhs);


}
