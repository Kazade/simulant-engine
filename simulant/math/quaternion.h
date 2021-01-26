#pragma once

#include "degrees.h"
#include "radians.h"
#include "euler.h"
#include "utils.h"
#include "vec3.h"

namespace smlt {

struct Vec3;
struct Mat3;
struct Mat4;

struct AxisAngle {
    Vec3 axis;
    Degrees angle;
};

struct Quaternion {
    friend struct Vec3;
    friend struct Mat4;
    friend struct Mat3;

    float x;
    float y;
    float z;
    float w;

    Quaternion():
        x(0), y(0), z(0), w(1) {

    }

    Quaternion(const Degrees& pitch, const Degrees& yaw, const Degrees& roll);

    Quaternion(const Vec3& axis, const Degrees& degrees);
    Quaternion(const Mat3& rot_matrix);

    Quaternion(float x, float y, float z, float w):
        x(x), y(y), z(z), w(w) {

    }

    Euler to_euler() const;

    AxisAngle to_axis_angle() const;

    float length_squared() const {
        return (x * x + y * y + z * z) + (w * w);
    }

    float length() const {
        return sqrtf(length_squared());
    }

    void normalize() {
        float l = 1.0f / length();
        x *= l;
        y *= l;
        z *= l;
        w *= l;
    }

    const Quaternion normalized() {
        Quaternion result = *this;
        result.normalize();
        return result;
    }

    Quaternion conjugated() {
        return Quaternion(-x, -y, -z, w);
    }

    float dot(const Quaternion& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    void inverse() {
        float d = dot(*this);
        *this = conjugated() / d;
    }

    const Quaternion inversed() const {
        Quaternion result(*this);
        result.inverse();
        return result;
    }

    bool equals(const Quaternion& rhs) const {
        return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
    }

    bool operator==(const Quaternion& rhs) const {
        return std::abs(dot(rhs)) > (1.0f - EPSILON);
    }

    bool operator!=(const Quaternion& rhs) const {
        return !(*this == rhs);
    }

    Quaternion& operator*=(const Quaternion& rhs) {
        const Quaternion p(*this);
        const Quaternion q(rhs);

        w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
        x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
        y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
        z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
        return *this;
    }

    Quaternion operator*(const Quaternion& rhs) const {
        return Quaternion(*this) *= rhs;
    }

    Quaternion& operator+=(const Quaternion& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    Quaternion operator+(const Quaternion& rhs) const {
        return Quaternion(*this) += rhs;
    }

    Quaternion& operator*=(const float rhs) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;

        return *this;
    }

    Quaternion operator*(const float rhs) const {
        return Quaternion(*this) *= rhs;
    }

    Quaternion operator/(const float rhs) const {
        float l = 1.0f / rhs;
        return Quaternion(*this) *= l;
    }

    Vec3 axis() const {
        auto tmp1 = 1.0f - w * w;
        if(tmp1 <= 0.0f) {
            return Vec3(0, 0, 1);
        }

        auto tmp2 = 1.0f / sqrtf(tmp1);
        return Vec3(x * tmp2, y * tmp2, z * tmp2);
    }

    Radians angle() const {
        return Radians(std::acos(w) * 2.0f);
    }

    Vec3 operator*(const Vec3& v) const;

    Quaternion operator-() const {
        return Quaternion(
            -x, -y, -z, -w
        );
    }

    Quaternion nlerp(const Quaternion& rhs, float t) const;
    Quaternion slerp(const Quaternion& rhs, float t) const;

    const Degrees pitch() const {
        return Radians(std::atan2(-2.0f * (y * z + w * x), w * w - x * x - y * y + z * z));
    }

    const Degrees yaw() const {
        return Radians(std::asin(clamp(-2.0f * (x * z - w * y), -1.0f, 1.0f)));
    }

    const Degrees roll() const {
        return Radians(std::atan2(2.0f * (x * y + w * z), w * w + x * x - y * y - z * z));
    }

    Vec3 forward() const {
        // OpenGL coordinate system has Neg-z as "forward"
        return Vec3::NEGATIVE_Z.rotated_by(*this);
    }

    Vec3 up() const {
        return Vec3::POSITIVE_Y.rotated_by(*this);
    }

    Vec3 right() const {
        return Vec3::POSITIVE_X.rotated_by(*this);
    }

    /* Returns the Quaternion rotation representing a turn to direction, using up as a basis.
     * If up and direction are colinear, or either are zero length, returns an identity
     * Quaternion */
    static Quaternion look_rotation(const Vec3& direction, const Vec3& up=Vec3(0, 1, 0));
};

Quaternion operator*(float s, const Quaternion& q);
std::ostream& operator<<(std::ostream& stream, const Quaternion& quat);

}
