#include "mat3.h"
#include "quaternion.h"
#include "mat4.h"

namespace smlt {

float Mat3::determinant() const {
    float output = m[0] * m[4] * m[8] + m[1] * m[5] * m[6] + m[2] * m[3] * m[7];
    output -= m[2] * m[4] * m[6] + m[0] * m[5] * m[7] + m[1] * m[3] * m[8];
    return output;
}

Mat3 Mat3::adjugate() const {
    Mat3 ret;

    ret.m[0] = m[4] * m[8] - m[5] * m[7];
    ret.m[1] = m[2] * m[7] - m[1] * m[8];
    ret.m[2] = m[1] * m[5] - m[2] * m[4];
    ret.m[3] = m[5] * m[6] - m[3] * m[8];
    ret.m[4] = m[0] * m[8] - m[2] * m[6];
    ret.m[5] = m[2] * m[3] - m[0] * m[5];
    ret.m[6] = m[3] * m[7] - m[4] * m[6];
    ret.m[7] = m[1] * m[6] - m[0] * m[7];
    ret.m[8] = m[0] * m[4] - m[1] * m[3];

    return ret;
}

void Mat3::inverse() {
    auto det = determinant();

    if(det == 0.0f) {
        return;
    }

    auto detInv = 1.0f / det;

    auto adj = adjugate();

    *this = adj * detInv;
}

Mat3 Mat3::as_rotation_x(const Degrees& angle) {
    Quaternion q(angle, Degrees(), Degrees());
    return Mat3::as_rotation(q);
}

Mat3 Mat3::as_rotation_y(const Degrees& angle) {
    Quaternion q(Degrees(), angle, Degrees());
    return Mat3::as_rotation(q);
}

Mat3 Mat3::as_rotation_z(const Degrees& angle) {
    Quaternion q(Degrees(), Degrees(), angle);
    return Mat3::as_rotation(q);
}

Mat3::Mat3(const Mat4 &rhs) {
    m[0] = rhs[0];
    m[1] = rhs[1];
    m[2] = rhs[2];

    m[3] = rhs[4];
    m[4] = rhs[5];
    m[5] = rhs[6];

    m[6] = rhs[8];
    m[7] = rhs[9];
    m[8] = rhs[10];
}

Mat3 Mat3::as_rotation(const Quaternion& q) {
    float xx(q.x * q.x);
    float yy(q.y * q.y);
    float zz(q.z * q.z);
    float xz(q.x * q.z);
    float xy(q.x * q.y);
    float yz(q.y * q.z);
    float wx(q.w * q.x);
    float wy(q.w * q.y);
    float wz(q.w * q.z);

    Mat3 result;

    result[0] = 1 - 2 * yy - 2 * zz;
    result[1] = 2 * xy - 2 * wz;
    result[2] = 2 * xz + 2 * wy;

    result[3] = 2 * xy + 2 * wz;
    result[4] = 1 - 2 * xx - 2 * zz;
    result[5] = 2 * yz - 2 * wx;

    result[6] = 2 * xz - 2 * wy;
    result[7] = 2 * yz + 2 * wx;
    result[8] = 1 - 2 * xx - 2 * yy;

    return result;
}

Mat3::Mat3(const Vec3& c0, const Vec3& c1, const Vec3& c2) {
    m[0] = c0.x;
    m[1] = c0.y;
    m[2] = c0.z;

    m[3] = c1.x;
    m[4] = c1.y;
    m[5] = c1.z;

    m[6] = c2.x;
    m[7] = c2.y;
    m[8] = c2.z;
}

Vec3 Mat3::transform_vector(const Vec3 &v) const {
    Vec3 ret;

    ret.x = v.x * m[0] + v.y * m[3] + v.z * m[6];
    ret.y = v.x * m[1] + v.y * m[4] + v.z * m[7];
    ret.z = v.x * m[2] + v.y * m[5] + v.z * m[8];

    return ret;
}

}
