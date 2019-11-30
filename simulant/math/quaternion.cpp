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

Quaternion::Quaternion(Degrees pitch, Degrees yaw, Degrees roll) {
    Quaternion x = Quaternion(Vec3::POSITIVE_X, pitch);
    Quaternion y = Quaternion(Vec3::POSITIVE_Y, yaw);
    Quaternion z = Quaternion(Vec3::POSITIVE_Z, roll);

    *this = z * x * y;
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
        w = 0.5 * s;
        s = 0.5 / s;
        x = (m21 - m12) * s;
        y = (m02 - m20) * s;
        z = (m10 - m01) * s;
    } else if ((m00 > m11) && (m00 > m22)) { // by x
        float s = sqrt(1 + m00 - m11 - m22);
        x = s * 0.5;
        s = 0.5 / s;
        y = (m10 + m01) * s;
        z = (m02 + m20) * s;
        w = (m21 - m12) * s;
    } else if (m11 > m22) { // by y
        float s = sqrt(1 + m11 - m00 - m22);
        y = s * 0.5;
        s = 0.5 / s;
        x = (m10 + m01) * s;
        z = (m21 + m12) * s;
        w = (m02 - m20) * s;
    } else { // by z
        float s = sqrt(1 + m22 - m00 - m11);
        z = s * 0.5;
        s = 0.5 / s;
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

    const constexpr float EPSILON = std::numeric_limits<float>::epsilon();

    // Lerp to avoid side effect of sin(angle) becoming a zero denominator
    if(cos_theta > 1.0f - EPSILON) {
        // Linear interpolation
        return Quaternion(
            lerp(this->x, z.x, t),
            lerp(this->y, z.y, t),
            lerp(this->z, z.z, t),
            lerp(this->w, z.w, t)
        );
    } else {
        auto angle = acos(cos_theta);
        return (sinf((1.0f - t) * angle) * (*this) + sinf(t * angle) * z) / sinf(angle);
    }
}

Quaternion operator*(float s, const Quaternion &q) {
    return q * s;
}

}
