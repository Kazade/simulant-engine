#pragma once

#include <cmath>
#include "../utils/unicode.h"
#include "../utils/formatter.h"
#include "utils.h"

namespace smlt {

typedef std::vector<float> FloatArray;

struct Vec3;
struct Vec4;
struct Degrees;

struct Vec2 {

public:
    static Vec2 left() {
        return Vec2(-1.0f, 0.0f);
    }

    static Vec2 right() {
        return Vec2(1.0f, 0.0f);
    }

    static Vec2 up() {
        return Vec2(0.0f, 1.0f);
    }

    static Vec2 down() {
        return Vec2(0.0f, -1.0f);
    }

    static Vec2 zero() {
        return Vec2(0, 0);
    }

    static Vec2 one() {
        return Vec2(1, 1);
    }

    float x;
    float y;

    Vec2() :
        x(0.0f), y(0.0f) {}

    Vec2(const FloatArray& arr) :
        x(arr[0]), y(arr[1]) {}

    Vec2(float x, float y):
        x(x), y(y) {

    }

    Vec2 rotated_by(const Degrees& degrees) const;

    float length() const  {
        return fast_sqrt(x * x + y * y);
    }

    float length_squared() const {
#ifdef __DREAMCAST__
        return MATH_Sum_of_Squares(x, y, 0.0f, 0.0f);
#else
        return dot(*this);
#endif
    }

    void normalize() {
        float l = fast_inverse_sqrt(length_squared());
        x *= l;
        y *= l;
    }

    Vec2 normalized() const {
        Vec2 ret = *this;
        ret.normalize();
        return ret;
    }

    Vec2 lerp(const Vec2& end, float t) const {
        t = fast_min(t, 1.0f);
        t = fast_max(t, 0.0f);

        return Vec2(
            fast_fmaf((end.x - x), t, x),
            fast_fmaf((end.y - y), t, y)
        );
    }

    Vec2 lerp_smooth(const Vec2& end, const float dt, const float p, const float t) const {
        return Vec2(
            fast_fmaf((end.x - x), 1.0f - ::powf(p, fast_divide(dt, t)), x),
            fast_fmaf((end.y - y), 1.0f - ::powf(p, fast_divide(dt, t)), y)
        );
    }

    void limit(float l) {
        if(length() > l) {
            normalize();
            *this *= l;
        }
    }

    operator FloatArray() const {
        return {x, y};
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
        float l = fast_divide(1.0f, rhs);
        *this *= l;
        return *this;
    }

    Vec2 operator/(float rhs) const {
        float l = fast_divide(1.0f, rhs);
        Vec2 result(x * l, y * l);
        return result;
    }

    Vec2 operator-() const {
        return Vec2(-x, -y);
    }

    Vec2 operator-(const Vec2& rhs) const {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    float dot(const Vec2& rhs) const {
#ifdef __DREAMCAST__
        return MATH_fipr(x, y, 0.0f, 0.0f, rhs.x, rhs.y, 0.0f, 0.0f);
#else
        return x * rhs.x + y * rhs.y;
#endif
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
