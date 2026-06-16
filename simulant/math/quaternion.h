#pragma once

#include "degrees.h"
#include "euler.h"
#include "lerp.h"
#include "radians.h"
#include "utils.h"
#include "vec3.h"

namespace smlt {

typedef std::vector<float> FloatArray;

struct Vec3;
struct Mat3;
struct Mat4;

struct AxisAngle {
    Vec3 axis;
    Degrees angle;
};


struct alignas(8) Quaternion {
    friend struct Vec3;
    friend struct Mat4;
    friend struct Mat3;

    float w, x, y, z;

    Quaternion() {
        w = 1.0f;
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    Quaternion(const FloatArray& arr) {
        w = arr[3];
        x = arr[0];
        y = arr[1];
        z = arr[2];
    }

    Quaternion(const Degrees& pitch, const Degrees& yaw, const Degrees& roll);

    Quaternion(const Vec3& axis, const Degrees& degrees);
    Quaternion(const Mat3& rot_matrix);

    Quaternion(const Euler& angles) : Quaternion(Degrees(angles.x), Degrees(angles.y), Degrees(angles.z)) {

    }

    Quaternion(float x, float y, float z, float w) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    Euler to_euler() const;

    AxisAngle to_axis_angle() const;

    float length_squared() const {
        return dot(*this);
    }

    float length() const {
        return fast_sqrt(length_squared());
    }

    void normalize();

    const Quaternion normalized() const {
        Quaternion result = *this;
        result.normalize();
        return result;
    }

    Quaternion conjugated() {
        return Quaternion(-x, -y, -z, w);
    }

    float dot(const Quaternion& rhs) const;

    void inverse();

    const Quaternion inversed() const {
        Quaternion result(*this);
        result.inverse();
        return result;
    }

    operator FloatArray() const {
        return {x, y, z, w};
    }

    bool equals(const Quaternion& rhs) const {
        return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
    }

    bool operator==(const Quaternion& rhs) const {
        return almost_equal(x, rhs.x) && almost_equal(y, rhs.y) && almost_equal(z, rhs.z) && almost_equal(w, rhs.w);
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

    Vec3 axis() const;

    Radians angle() const {
        return Radians(std::acos(w) * 2.0f);
    }

    Quaternion operator-() const {
        return Quaternion(
            -x, -y, -z, -w
        );
    }

    Quaternion nlerp(const Quaternion& rhs, float t) const;
    Quaternion slerp(const Quaternion& rhs, float t) const;

    const Degrees pitch() const;
    const Degrees yaw() const;
    const Degrees roll() const;

    Vec3 forward() const {
        // OpenGL coordinate system has Neg-z as "forward"
        return Vec3::forward().rotated_by(*this);
    }

    Vec3 up() const {
        return Vec3::up().rotated_by(*this);
    }

    Vec3 right() const {
        return Vec3::right().rotated_by(*this);
    }

    /* Returns the Quaternion rotation representing a turn to direction, using up as a basis.
     * If up and direction are colinear, or either are zero length, returns an identity
     * Quaternion */
    static Quaternion look_rotation(const Vec3& direction, const Vec3& up=Vec3(0, 1, 0));
};

Quaternion operator*(float s, const Quaternion& q);
std::ostream& operator<<(std::ostream& stream, const Quaternion& quat);

}
