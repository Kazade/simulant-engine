#include "mat4.h"
#include "mat3.h"
#include "../types.h"
#include "utils.h"

namespace smlt {

Mat4::Mat4(const Quaternion &rhs) {
    Mat3 tmp(rhs);

    m[0] = tmp[0];
    m[1] = tmp[1];
    m[2] = tmp[2];
    m[3] = 0.0f;

    m[4] = tmp[3];
    m[5] = tmp[4];
    m[6] = tmp[5];
    m[7] = 0.0f;

    m[8] = tmp[6];
    m[9] = tmp[7];
    m[10] = tmp[8];
    m[11] = 0.0f;

    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}

Mat4::Mat4(const Quaternion& rot, const Vec3& trans, const Vec3& scale):
    Mat4(rot) {

    m[12] = trans.x;
    m[13] = trans.y;
    m[14] = trans.z;

    m[0] = scale.x;
    m[5] = scale.y;
    m[10] = scale.z;
}


Vec4 Mat4::operator*(const Vec4 &v) const {
    Vec4 ret;
    ret.x = v.x * m[0] + v.y * m[4] + v.z * m[8] + v.w * m[12];
    ret.y = v.x * m[1] + v.y * m[5] + v.z * m[9] + v.w * m[13];
    ret.z = v.x * m[2] + v.y * m[6] + v.z * m[10] + v.w * m[14];
    ret.w = v.x * m[3] + v.y * m[7] + v.z * m[11] + v.w * m[15];
    return ret;
}

void Mat4::extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) const {
    Mat3 rot(*this);
    rotation = Quaternion(rot);

    translation.x = m[12];
    translation.y = m[13];
    translation.z = m[14];
}

Mat4 Mat4::as_rotation_x(const Degrees &angle) {
    Quaternion rot(angle, Degrees(), Degrees());
    return Mat4(rot);
}

Mat4 Mat4::as_rotation_y(const Degrees &angle) {
    Quaternion rot(Degrees(), angle, Degrees());
    return Mat4(rot);
}

Mat4 Mat4::as_rotation_z(const Degrees &angle) {
    Quaternion rot(Degrees(), Degrees(), angle);
    return Mat4(rot);
}

Mat4 Mat4::as_scaling(float s) {
    Mat4 ret;
    ret[0] = s;
    ret[5] = s;
    ret[10] = s;
    return ret;
}

Mat4 Mat4::from_pos_rot_scale(const Vec3 &pos, const Quaternion &rot, const Vec3 &scale) {
    Mat4 s;
    Mat4 t;
    Mat4 r(rot);

    s[0] = scale.x;
    s[5] = scale.y;
    s[10] = scale.z;

    t[12] = pos.x;
    t[13] = pos.y;
    t[14] = pos.z;

    return t * r * s;
}

Mat4 Mat4::as_translation(const Vec3 &v) {
    Mat4 ret;
    ret[12] = v.x;
    ret[13] = v.y;
    ret[14] = v.z;

    return ret;
}

Mat4 Mat4::as_projection(const Degrees &fov, float aspect, float zNear, float zFar) {
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

    auto fovy = Radians(fov).value;

    float const tanHalfFovy = std::tan(fovy * 0.5f);

    Mat4 result;

    result[0] = 1.0f / (aspect * tanHalfFovy);
    result[5] = 1.0f / (tanHalfFovy);
    result[10] = - (zFar + zNear) / (zFar - zNear);
    result[11] = -1.0f;
    result[14] = - (2.0f * zFar * zNear) / (zFar - zNear);
    result[15] = 0.0f;

    return result;
}

Mat4 Mat4::as_orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {
    Mat4 result;

    result[0] = 2.0f / (right - left);
    result[5] = 2.0f / (top - bottom);
    result[10] = -2.0f / (zFar - zNear);

    result[12] = - (right + left) / (right - left);
    result[13] = - (top + bottom) / (top - bottom);
    result[14] = - (zFar + zNear) / (zFar - zNear);

    return result;
}

void Mat4::inverse() {
    Mat4 tmp;

    tmp.m[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    tmp.m[4] = -m[4]  * m[10] * m[15] +
              m[4]  * m[11] * m[14] +
              m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] -
              m[12] * m[6]  * m[11] +
              m[12] * m[7]  * m[10];

    tmp.m[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    tmp.m[12] = -m[4]  * m[9] * m[14] +
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    tmp.m[1] = -m[1]  * m[10] * m[15] +
              m[1]  * m[11] * m[14] +
              m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] -
              m[13] * m[2] * m[11] +
              m[13] * m[3] * m[10];

    tmp.m[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    tmp.m[9] = -m[0]  * m[9] * m[15] +
              m[0]  * m[11] * m[13] +
              m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] -
              m[12] * m[1] * m[11] +
              m[12] * m[3] * m[9];

    tmp.m[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    tmp.m[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    tmp.m[6] = -m[0]  * m[6] * m[15] +
              m[0]  * m[7] * m[14] +
              m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] -
              m[12] * m[2] * m[7] +
              m[12] * m[3] * m[6];

    tmp.m[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    tmp.m[14] = -m[0]  * m[5] * m[14] +
               m[0]  * m[6] * m[13] +
               m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] -
               m[12] * m[1] * m[6] +
               m[12] * m[2] * m[5];

    tmp.m[3] = -m[1] * m[6] * m[11] +
              m[1] * m[7] * m[10] +
              m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] -
              m[9] * m[2] * m[7] +
              m[9] * m[3] * m[6];

    tmp.m[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    tmp.m[11] = -m[0] * m[5] * m[11] +
               m[0] * m[7] * m[9] +
               m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] -
               m[8] * m[1] * m[7] +
               m[8] * m[3] * m[5];

    tmp.m[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    float det = m[0] * tmp.m[0] + m[1] * tmp.m[4] + m[2] * tmp.m[8] + m[3] * tmp.m[12];

    if (det == 0.0f) {
        return;
    }

    det = fast_divide(1.0f, det);

    for (uint8_t i = 0; i < 16; i++) {
        m[i] = tmp.m[i] * det;
    }
}

Plane Mat4::extract_plane(FrustumPlane plane) const {
    float t = 1.0f;
    Plane out;

    switch(plane) {
    case FRUSTUM_PLANE_RIGHT:
        out.n.x = m[3] - m[0];
        out.n.y = m[7] - m[4];
        out.n.z = m[11] - m[8];
        out.d = m[15] - m[12];
        break;
    case FRUSTUM_PLANE_LEFT:
        out.n.x = m[3] + m[0];
        out.n.y = m[7] + m[4];
        out.n.z = m[11] + m[8];
        out.d = m[15] + m[12];
        break;
    case FRUSTUM_PLANE_BOTTOM:
        out.n.x = m[3] + m[1];
        out.n.y = m[7] + m[5];
        out.n.z = m[11] + m[9];
        out.d = m[15] + m[13];
        break;
    case FRUSTUM_PLANE_TOP:
        out.n.x = m[3] - m[1];
        out.n.y = m[7] - m[5];
        out.n.z = m[11] - m[9];
        out.d = m[15] - m[13];
        break;
    case FRUSTUM_PLANE_FAR:
        out.n.x = m[3] - m[2];
        out.n.y = m[7] - m[6];
        out.n.z = m[11] - m[10];
        out.d = m[15] - m[14];
        break;
    case FRUSTUM_PLANE_NEAR:
        out.n.x = m[3] + m[2];
        out.n.y = m[7] + m[6];
        out.n.z = m[11] + m[10];
        out.d = m[15] + m[14];
        break;
    default:
        assert(0 && "Invalid plane index");
    }

#ifdef __DREAMCAST__
    t = MATH_Sum_of_Squares(out.n.x, out.n.y, out.n.z, 0.0f);
#else
    t = out.n.dot(out.n);
#endif

    out.n *= fast_inverse_sqrt(t);
    out.d *= fast_inverse_sqrt(t);

    return out;
}

Mat4 Mat4::as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
    /* If the up vector is parellel to the view vector then we swap the up Y/Z axis */

    float d = up.dot((target - eye));
    auto tu = (almost_equal(d * d, 1.0f)) ? Vec3(up.x, up.z, up.y) : up;

    Vec3 f = (target - eye).normalized();
    Vec3 s = f.cross(tu).normalized();
    Vec3 u = s.cross(f);

    Mat4 ret;

    ret.m[0] = s.x;
    ret.m[1] = u.x;
    ret.m[2] = -f.x;
    ret.m[3] = 0.0;

    ret.m[4] = s.y;
    ret.m[5] = u.y;
    ret.m[6] = -f.y;
    ret.m[7] = 0.0;

    ret.m[8] = s.z;
    ret.m[9] = u.z;
    ret.m[10] = -f.z;
    ret.m[11] = 0.0;

    ret.m[12] = -s.dot(eye);
    ret.m[13] = -u.dot(eye);
    ret.m[14] = f.dot(eye);
    ret.m[15] = 1.0;

    return ret;
}

}
