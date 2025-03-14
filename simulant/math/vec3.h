#pragma once

#include <cmath>
#include "../utils/unicode.h"
#include "../utils/formatter.h"
#include "utils.h"

#ifdef __DREAMCAST__
#include <kos.h>
#endif

namespace smlt {

typedef std::vector<float> FloatArray;

struct Vec2;
struct Quaternion;
struct Mat4;
struct Degrees;
struct Mat3;
struct Mat4;
struct Vec4;
class AABB;

struct Vec3 {
private:
    friend struct Quaternion;
    friend struct Mat4;
    friend struct Mat3;
    friend struct Vec2;
    friend struct Ray;

public:
    static Vec3 up() {
        return Vec3(0.0f, 1.0f, 0.0f);
    }

    static Vec3 down() {
        return Vec3(0.0f, -1.0f, 0.0f);
    }

    static Vec3 left() {
        return Vec3(-1.0f, 0.0f, 0.0f);
    }

    static Vec3 right() {
        return Vec3(1.0f, 0.0f, 0.0f);
    }

    static Vec3 forward() {
        return Vec3(0.0f, 0.0f, -1.0f);
    }

    static Vec3 backward() {
        return Vec3(0.0f, 0.0f, 1.0f);
    }

    static Vec3 zero() {
        return Vec3(0, 0, 0);
    }

    static Vec3 one() {
        return Vec3(1, 1, 1);
    }

    float x;
    float y;
    float z;

    Vec3():
        x(0.0f),
        y(0.0f),
        z(0.0f) {
    }

    Vec3(const FloatArray& arr) :
        x(arr[0]), y(arr[1]), z(arr[2]) {}

    Vec3(float xyz):
        x(xyz), y(xyz), z(xyz) {}

    Vec3(float x, float y):
        x(x), y(y), z(0) {}

    Vec3(float x, float y, float z):
        x(x), y(y), z(z) {
    }

    Vec3(const Vec2& v2, float z);
    Vec3(const Vec3& v):
        x(v.x), y(v.y), z(v.z) {}

    Vec3& operator=(const Vec3& rhs) {
        if(&rhs == this) {
            return *this;
        }

        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
        return *this;
    }

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vec3& operator+=(const Vec3& rhs) {
        *this = *this + rhs;
        return *this;
    }

    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3& operator-=(const Vec3& rhs) {
        *this = *this - rhs;
        return *this;
    }

    Vec3 operator*(const Vec3& rhs) const {
        return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    Vec3 operator*(float rhs) const {
        Vec3 result(x * rhs, y * rhs, z * rhs);
        return result;
    }

    Vec3& operator*=(float rhs) {
        *this = *this * rhs;
        return *this;
    }

    Vec3& operator/=(float rhs) {
        float l = fast_divide(1.0f, rhs);
        *this *= l;
        return *this;
    }

    Vec3 operator/(float rhs) const {
        float l = fast_divide(1.0f, rhs);
        Vec3 result(x * l, y * l, z * l);
        return result;
    }

    Vec3 operator/(const Vec3& rhs) const {
        return Vec3(x / rhs.x, y / rhs.y, z / rhs.z);
    }

    bool equals(const Vec3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    bool operator==(const Vec3& rhs) const {
        return (*this - rhs).length_squared() < EPSILON;
    }

    bool operator!=(const Vec3& rhs) const {
        return !(*this == rhs);
    }

    void set(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    float length_squared() const {
        return dot(*this);
    }

    float length() const {
        return fast_sqrt(length_squared());
    }

    void normalize() {
        float l = fast_inverse_sqrt(length_squared());
        x *= l;
        y *= l;
        z *= l;
    }

    const Vec3 normalized() const {
        Vec3 result = *this;
        result.normalize();
        return result;
    }

    Vec3 lerp(const Vec3& end, float t) const {
        t = fast_min(t, 1.0f);
        t = fast_max(t, 0.0f);

        return Vec3(
            fast_fmaf((end.x - x), t, x),
            fast_fmaf((end.y - y), t, y),
            fast_fmaf((end.z - z), t, z)
        );
    }

    Vec3 lerp_smooth(const Vec3& end, const float dt, const float p, const float t) const {
        return Vec3(
            fast_fmaf((end.x - x), 1.0f - std::pow(p, fast_divide(dt, t)), x),
            fast_fmaf((end.y - y), 1.0f - std::pow(p, fast_divide(dt, t)), y),
            fast_fmaf((end.z - z), 1.0f - std::pow(p, fast_divide(dt, t)), z)
        );
    }

    // Exponential decay function based on Freya Holmer's talk
    // Reference: https://www.youtube.com/watch?v=LSNQuFEDOyQ
    Vec3 lerp_decay(const Vec3& end, const float dt, const float decay) const {
        const float expDecay = std::exp(-decay * dt);

        return Vec3(
            fast_fmaf((x - end.x), expDecay, end.x),
            fast_fmaf((y - end.y), expDecay, end.y),
            fast_fmaf((z - end.z), expDecay, end.z)
        );
    }

    Vec3 rotated_by(const Quaternion& q) const;

    Vec3 rotated_by(const Mat3& rot) const;

    /* Note: We do *not* supply multiply operators with Mat4 because
     * the behaviour you want depends on whether want translation or not
     * (e.g. is this a point in space or a vector). Instead we provide
     * rotated_by() and transformed_by() the former assumes a W component of
     * 0 whereas the latter assumes a W component of 1.0
     */

    Vec3 rotated_by(const Mat4& rot) const;

    Vec3 transformed_by(const Mat4& trans) const;

    Vec3 project_onto_vec3(const Vec3& projection_target) const {
        Vec3 n = projection_target.normalized();
        return n * dot(n);
    }

    float dot(const Vec3& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    Vec3 cross(const Vec3& rhs) const {
        Vec3 ret;

        float a = (z * rhs.y);  // fmul
        float b = (x * rhs.z);  // fmul
        a = -a;  // fneg
        float c = (y * rhs.x);  // fmul
        b = -b;  // fneg
        ret.x = fast_fmaf(y, rhs.z, a);  // fmac
        c = -c;  // fneg
        ret.y = fast_fmaf(z, rhs.x, b); // fmac
        ret.z = fast_fmaf(x, rhs.y, c);  // fmac

        return ret;
    }

    Vec3 limit(float l) {
        if(length() > l) {
            normalize();
            *this *= l;
        }

        return *this;
    }

    float distance_to(const AABB& aabb) const;
    float distance_to(const Vec3& other) const {
        return (other - *this).length();
    }

    float squared_distance_to(const Vec3& other) const {
        return (other - *this).length_squared();
    }

    template<typename Container>
    static Vec3 find_average(const Container& vectors) {
        Vec3 ret;
        for(auto& v: vectors) {
            ret += v;
        }

        ret /= float(vectors.size());
        return ret;
    }

    inline Vec3 parallel_component(const Vec3& unit_basis) const {
        const float projection = this->dot(unit_basis);
        return unit_basis * projection;
    }

    // return component of vector perpendicular to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))

    inline Vec3 perpendicular_component (const Vec3& unit_basis) const {
        return (*this) - parallel_component(unit_basis);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vec3& vec);

    Vec3 perpendicular() const;
    Vec3 random_deviant(const Degrees& angle) const;

    Vec2 xy() const;
    Vec4 xyzw(float w=1.0f) const;

    /* Returns the Quaternion rotation between this vector
     * and `dir` */
    Quaternion rotation_to(const Vec3& dir) const;

    static Vec3 min(const Vec3& a, const Vec3& b) {
        return Vec3(fast_min(a.x, b.x), fast_min(a.y, b.y), fast_min(a.z, b.z));
    }

    static Vec3 max(const Vec3& a, const Vec3& b) {
        return Vec3(fast_max(a.x, b.x), fast_max(a.y, b.y), fast_max(a.z, b.z));
    }

    operator FloatArray() const {
        return {x, y, z};
    }
};

std::ostream& operator<<(std::ostream& stream, const Vec3& vec);

Vec3 operator*(float lhs, const Vec3& rhs);
Vec3 operator/(float lhs, const Vec3& rhs);
Vec3 operator-(const Vec3& vec);

}
