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

    Quaternion(Degrees pitch, Degrees yaw, Degrees roll);

    Quaternion(const Vec3& axis, const Degrees& degrees);
    Quaternion(const Mat3& rot_matrix);

    Quaternion(float x, float y, float z, float w):
        x(x), y(y), z(z), w(w) {

    }

    Vec3 rotate_vector(const Vec3& v) const;

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

    Vec3 operator*(const Vec3& v) const {
        const Vec3 quat_vector(x, y, z);
        const Vec3 uv = quat_vector.cross(v);
        const Vec3 uuv = quat_vector.cross(uv);

        return v + ((uv * w) + uuv) * 2.0f;
    }

    Quaternion operator-() const {
        return Quaternion(
            -x, -y, -z, -w
        );
    }

    Quaternion nlerp(const Quaternion& rhs, float t);
    Quaternion slerp(const Quaternion& rhs, float t);

    const Degrees pitch() const {
        return Radians(atan2(2.0f * (y * z + w * x), w * w - x * x - y * y + z * z));
    }

    const Degrees yaw() const {
        return Radians(asin(clamp(-2.0f * (x * z - w * y), -1.0f, 1.0f)));
    }

    const Degrees roll() const {
        return Radians(atan2(2.0f * (x * y + w * z), w * w + x * x - y * y - z * z));
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

    static Quaternion as_look_at(const Vec3& direction, const Vec3& up);
};

Quaternion operator*(float s, const Quaternion& q);
std::ostream& operator<<(std::ostream& stream, const Quaternion& quat);

}
