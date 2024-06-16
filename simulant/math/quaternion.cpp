#include "../types.h"
#include "quaternion.h"

namespace smlt {

std::ostream& operator<<(std::ostream& stream, const Quaternion& quat) {
    stream << "(" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << ")";
    return stream;
}

Quaternion Quaternion::look_rotation(const Vec3& direction, const Vec3& up) {
    // float d = fast_abs(up.dot(direction));
    // if(almost_equal(d, 1.0f)) {
    //     return Quaternion();
    // }

    // if(almost_equal(direction.length_squared(), 0.0f) ||
    //     almost_equal(up.length_squared(), 0.0f)) {
    //     return Quaternion();
    // }

    // Vec3 t = up.cross(-direction).normalized();
    // auto ret = Quaternion(Mat3(t, -direction.cross(t), -direction));

    auto forward = direction.normalized();
    auto right = forward.cross(up).normalized();
    auto up_ = right.cross(forward);

    Mat3 rot;
    rot[0] = right.x;
    rot[1] = right.y;
    rot[2] = right.z;

    rot[3] = up_.x;
    rot[4] = up_.y;
    rot[5] = up_.z;

    rot[6] = -forward.x;
    rot[7] = -forward.y;
    rot[8] = -forward.z;

    return Quaternion(rot);
}

Quaternion::Quaternion(const Degrees &pitch, const Degrees &yaw, const Degrees &roll) {
    const float p = smlt::Radians(pitch).to_float() * 0.5f;
    const float ya = smlt::Radians(yaw).to_float() * 0.5f;
    const float r = smlt::Radians(roll).to_float() * 0.5f;

    const float cp = std::sin(p);
    const float sp = std::cos(p);
    const float cy = std::sin(ya);
    const float sy = std::cos(ya);
    const float cr = std::sin(r);
    const float sr = std::cos(r);

    x = sp * cy * cr - cp * sy * sr;
    y = cp * sy * cr + sp * cy * sr;
    z = cp * cy * sr - sp * sy * cr;
    w = cp * cy * cr + sp * sy * sr;
}

Quaternion::Quaternion(const Vec3 &axis, const Degrees &degrees) {
    auto half_rad = Radians(degrees).to_float() * 0.5f;
    float factor = 0.0f;
    fast_sincos(half_rad, &factor, &w);

    x = axis.x * factor;
    y = axis.y * factor;
    z = axis.z * factor;

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
    if (t > 0) { // by w
        float s = 0.5f / fast_sqrt(t + 1.0f);
        w = fast_divide(0.25f, s);
        x = (m12 - m21) * s;
        y = (m20 - m02) * s;
        z = (m01 - m10) * s;
    } else if ((m00 > m11) && (m00 > m22)) { // by x
        float s = 2.0f * fast_sqrt(1.0f + m00 - m11 - m22);
        w = (m12 - m21) / s;
        x = 0.25f * s;
        y = (m10 + m01) / s;
        z = (m20 + m02) / s;
    } else if (m11 > m22) { // by y
        float s = 2.0f * fast_sqrt(1.0f + m11 - m00 - m22);
        w = (m20 - m02) / s;
        x = (m10 + m01) / s;
        y = 0.25f * s;
        z = (m21 + m12) / s;
    } else { // by z
        float s = 2.0f * fast_sqrt(1.0f + m22 - m00 - m11);
        w = (m01 - m10) / s;
        x = (m20 + m02) / s;
        y = (m21 + m12) / s;
        z = 0.25f * s;
    }
}

Euler Quaternion::to_euler() const {
    return Euler(
        Degrees(pitch()).to_float(),
        Degrees(yaw()).to_float(),
        Degrees(roll()).to_float()
    );
}

AxisAngle Quaternion::to_axis_angle() const {
    AxisAngle ret;
    ret.axis = axis();
    ret.angle = Degrees(angle());
    return ret;
}

Quaternion operator*(float s, const Quaternion &q) {
    return q * s;
}

}
