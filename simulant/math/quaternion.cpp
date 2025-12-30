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

Quaternion::Quaternion(const Mat3& matrix) {
    float trace = matrix[0] + matrix[4] + matrix[8];
    float S;

    if(trace > 0.0f) {
        S = 0.5f / std::sqrt(trace + 1.0f);
        w = 0.25f / S;
        x = (matrix[5] - matrix[7]) * S;
        y = (matrix[6] - matrix[2]) * S;
        z = (matrix[1] - matrix[3]) * S;
    } else {
        if(matrix[0] > matrix[4] && matrix[0] > matrix[8]) {
            S = 2.0f * std::sqrt(1.0f + matrix[0] - matrix[4] - matrix[8]);
            w = (matrix[5] - matrix[7]) / S;
            x = 0.25f * S;
            y = (matrix[1] + matrix[3]) / S;
            z = (matrix[6] + matrix[2]) / S;
        } else if(matrix[4] > matrix[8]) {
            S = 2.0f * std::sqrt(1.0f + matrix[4] - matrix[0] - matrix[8]);
            w = (matrix[6] - matrix[2]) / S;
            x = (matrix[1] + matrix[3]) / S;
            y = 0.25f * S;
            z = (matrix[5] + matrix[7]) / S;
        } else {
            S = 2.0f * std::sqrt(1.0f + matrix[8] - matrix[0] - matrix[4]);
            w = (matrix[1] - matrix[3]) / S;
            x = (matrix[6] + matrix[2]) / S;
            y = (matrix[5] + matrix[7]) / S;
            z = 0.25f * S;
        }
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
