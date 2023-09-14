#include "../types.h"
#include "quaternion.h"

namespace smlt {

std::ostream& operator<<(std::ostream& stream, const Quaternion& quat) {
    stream << "(" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << ")";
    return stream;
}

Quaternion Quaternion::look_rotation(const Vec3& direction, const Vec3& up) {
    float d = fast_abs(up.dot(direction));
    if(almost_equal(d, 1.0f)) {
        return Quaternion();
    }

    if(almost_equal(direction.length_squared(), 0.0f) ||
        almost_equal(up.length_squared(), 0.0f)) {
        return Quaternion();
    }

    Vec3 t = up.cross(-direction).normalized();
    auto ret = Quaternion(Mat3(t, -direction.cross(t), -direction));
    return ret;
}

Quaternion::Quaternion(const Degrees &pitch, const Degrees &yaw, const Degrees &roll) {
    const float p = smlt::Radians(pitch).value * 0.5f;
    const float ya = smlt::Radians(yaw).value * 0.5f;
    const float r = smlt::Radians(roll).value * 0.5f;

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
    auto half_rad = Radians(degrees).value * 0.5f;
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
        float root = fast_sqrt(t + 1.0f);
        w = 0.5f * root;
        root = fast_divide(0.5f, root);
        x = (m21 - m12) * root;
        y = (m02 - m20) * root;
        z = (m10 - m01) * root;
    } else if ((m00 > m11) && (m00 > m22)) { // by x
        float s = fast_sqrt(1 + m00 - m11 - m22);
        x = s * 0.5f;
        s = fast_divide(0.5f, s);
        y = (m10 + m01) * s;
        z = (m02 + m20) * s;
        w = (m21 - m12) * s;
    } else if (m11 > m22) { // by y
        float s = fast_sqrt(1 + m11 - m00 - m22);
        y = s * 0.5f;
        s = fast_divide(0.5f, s);
        x = (m10 + m01) * s;
        z = (m21 + m12) * s;
        w = (m02 - m20) * s;
    } else { // by z
        float s = fast_sqrt(1 + m22 - m00 - m11);
        z = s * 0.5f;
        s = fast_divide(0.5f, s);
        x = (m02 + m20) * s;
        y = (m21 + m12) * s;
        w = (m10 - m01) * s;
    }
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

Quaternion operator*(float s, const Quaternion &q) {
    return q * s;
}

}
