#include "../types.h"
#include "quaternion.h"

namespace smlt {

std::ostream& operator<<(std::ostream& stream, const Quaternion& quat) {
    stream << "(" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << ")";
    return stream;
}

Quaternion Quaternion::as_look_at(const Vec3& direction, const Vec3& up=Vec3(0, 1, 0)) {
    Mat4 lookat = Mat4::as_look_at(Vec3(), direction, up);
    Quaternion ret = Quaternion(Mat3((lookat)));
    return ret;
}

Quaternion::Quaternion(const Degrees &pitch, const Degrees &yaw, const Degrees &roll) {
    const float p = smlt::Radians(pitch).value * 0.5f;
    const float ya = smlt::Radians(yaw).value * 0.5f;
    const float r = smlt::Radians(roll).value * 0.5f;

    const float cp = std::cos(p);
    const float sp = std::sin(p);
    const float cy = std::cos(ya);
    const float sy = std::sin(ya);
    const float cr = std::cos(r);
    const float sr = std::sin(r);

    x = sp * cy * cr + cp * sy * sr;
    y = cp * sy * cr - sp * cy * sr;
    z = cp * cy * sr + sp * sy * cr;
    w = cp * cy * cr - sp * sy * sr;
}

Quaternion::Quaternion(const Vec3 &axis, const Degrees &degrees) {
    auto half_rad = Radians(degrees).value / 2.0f;
    auto factor = sinf(half_rad);

    x = axis.x * factor;
    y = axis.y * factor;
    z = axis.z * factor;
    w = cosf(half_rad);

    normalize();
}

Quaternion::Quaternion(const Mat3& rot_matrix) {
    float m12 = rot_matrix[7];
    float m21 = rot_matrix[5];
    float m02 = rot_matrix[6];
    float m20 = rot_matrix[2];
    float m10 = rot_matrix[1];
    float m01 = rot_matrix[3];
    float m00 = rot_matrix[0];
    float m11 = rot_matrix[4];
    float m22 = rot_matrix[8];
    float t = m00 + m11 + m22;
    // we protect the division by s by ensuring that s>=1
    if (t >= 0) { // by w
        float s = sqrt(t + 1);
        w = 0.5f * s;
        s = 0.5f / s;
        x = (m21 - m12) * s;
        y = (m02 - m20) * s;
        z = (m10 - m01) * s;
    } else if ((m00 > m11) && (m00 > m22)) { // by x
        float s = sqrt(1 + m00 - m11 - m22);
        x = s * 0.5f;
        s = 0.5f / s;
        y = (m10 + m01) * s;
        z = (m02 + m20) * s;
        w = (m21 - m12) * s;
    } else if (m11 > m22) { // by y
        float s = sqrt(1 + m11 - m00 - m22);
        y = s * 0.5f;
        s = 0.5f / s;
        x = (m10 + m01) * s;
        z = (m21 + m12) * s;
        w = (m02 - m20) * s;
    } else { // by z
        float s = sqrt(1 + m22 - m00 - m11);
        z = s * 0.5f;
        s = 0.5f / s;
        x = (m02 + m20) * s;
        y = (m21 + m12) * s;
        w = (m10 - m01) * s;
    }
}

Vec3 Quaternion::rotate_vector(const Vec3 &v) const {
    return (*this) * v;
}

Euler Quaternion::to_euler() const {
    return Euler(
        Degrees(pitch()).value,
        Degrees(yaw()).value,
        Degrees(roll()).value
    );
}

AxisAngle Quaternion::to_axis_angle() const {
    AxisAngle ret;
    ret.axis = axis();
    ret.angle = Degrees(angle());
    return ret;
}

Quaternion Quaternion::nlerp(const Quaternion &rhs, float t) {
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

Quaternion Quaternion::slerp(const Quaternion &rhs, float t) {
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

        auto s0 = std::cos(theta) - cos_theta * sin_theta / sin_theta_0;
        auto s1 = sin_theta / sin_theta_0;
        return (s0 * (*this)) + (s1 * z);
    }
}

Quaternion operator*(float s, const Quaternion &q) {
    return q * s;
}

}
