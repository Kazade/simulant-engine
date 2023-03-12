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

    Quaternion(const Euler& angles) : Quaternion(Degrees(angles.x), Degrees(angles.y), Degrees(angles.z)) {
    
    }

    Quaternion(float x, float y, float z, float w):
        x(x), y(y), z(z), w(w) {

    }

    Euler to_euler() const;

    AxisAngle to_axis_angle() const;

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
#ifdef __DREAMCAST__
        return MATH_fipr(x, y, z, w, rhs.x, rhs.y, rhs.z, rhs.w);
#else
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
#endif
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
        return fast_abs(dot(rhs)) > (1.0f - EPSILON);
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
        float l = fast_divide(1.0f, rhs);
        return Quaternion(*this) *= l;
    }

    Vec3 axis() const {
        auto tmp1 = 1.0f - w * w;
        if(tmp1 <= 0.0f) {
            return Vec3(0, 0, 1);
        }

        auto tmp2 = fast_inverse_sqrt(tmp1);
        return Vec3(x * tmp2, y * tmp2, z * tmp2);
    }

    Radians angle() const {
        return Radians(std::acos(w) * 2.0f);
    }

    Quaternion operator-() const {
        return Quaternion(
            -x, -y, -z, -w
        );
    }

    Quaternion nlerp(const Quaternion& rhs, float t) const {
        auto z = rhs;
        auto theta = this->dot(rhs);

        if(theta < 0.0f) {
            z = -rhs;
        }

        // Linear interpolation (result normalized)
        return Quaternion(
            lerp(this->x, z.x, t),
            lerp(this->y, z.y, t),
            lerp(this->z, z.z, t),
            lerp(this->w, z.w, t)
        ).normalized();
    }

    Quaternion slerp(const Quaternion& rhs, float t) const {
        auto z = rhs;

        auto cos_theta = this->dot(rhs);

        // negate to avoid interpolation taking long way around
        if (cos_theta < 0.0f) {
            z = -rhs;
            cos_theta = -cos_theta;
        }

        const constexpr float DOT_THRESHOLD = 0.9995f;

        // Lerp to avoid side effect of sin(angle) becoming a zero denominator
        if(cos_theta > DOT_THRESHOLD) {
            // Linear interpolation
            return Quaternion(
                lerp(this->x, z.x, t),
                lerp(this->y, z.y, t),
                lerp(this->z, z.z, t),
                lerp(this->w, z.w, t)
            ).normalized();
        } else {
            auto theta_0 = std::acos(cos_theta);
            auto theta = theta_0 * t;
            auto sin_theta = std::sin(theta);
            auto sin_theta_0 = std::sin(theta_0);

            auto s1 = fast_divide(sin_theta, sin_theta_0);
            auto s0 = std::cos(theta) - cos_theta * s1;

            return ((*this) * s0) + (z * s1);
        }
    }

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
