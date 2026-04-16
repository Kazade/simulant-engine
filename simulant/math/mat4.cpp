#include "mat4.h"
#include "../types.h"
#include "mat3.h"
#include "simulant/math/quaternion.h"
#include "utils.h"

namespace smlt {

Mat4 Mat4::as_rotation(const Quaternion& rhs) {
    Mat4 m;

    shz_quat q;
    q.x = rhs.x;
    q.y = rhs.y;
    q.z = rhs.z;
    q.w = rhs.w;

    shz_mat4x4_init_rotation_quat(&m.m, q);
    return m;
}

Vec4 Mat4::operator*(const Vec4& v) const {
    shz_vec4 vec;
    vec.x = v.x;
    vec.y = v.y;
    vec.z = v.z;
    vec.w = v.w;

    vec = shz_mat4x4_transform_vec4(&m, vec);
    return Vec4(vec.x, vec.y, vec.z, vec.w);
}

Vec3 Mat4::operator*(const Vec3& v) const {
    shz_vec3 vec;
    vec.x = v.x;
    vec.y = v.y;
    vec.z = v.z;

    vec = shz_mat4x4_transform_vec3(&m, vec);
    return Vec3(vec.x, vec.y, vec.z);
}

void Mat4::extract_rotation_and_translation(Quaternion& rotation,
                                            Vec3& translation) const {
    shz_quat q;
    shz_vec3 trn, scale;
    shz_mat4x4_decompose(&m, &trn, &q, &scale);

    rotation.x = q.x;
    rotation.y = q.y;
    rotation.z = q.z;
    rotation.w = q.w;

    translation.x = trn.x;
    translation.y = trn.y;
    translation.z = trn.z;
}

Mat4 Mat4::as_rotation_x(const Degrees& angle) {
    Quaternion rot(angle, Degrees(), Degrees());
    return Mat4::as_rotation(rot);
}

Mat4 Mat4::as_rotation_y(const Degrees& angle) {
    Quaternion rot(Degrees(), angle, Degrees());
    return Mat4::as_rotation(rot);
}

Mat4 Mat4::as_rotation_z(const Degrees& angle) {
    Quaternion rot(Degrees(), Degrees(), angle);
    return Mat4::as_rotation(rot);
}

Mat4 Mat4::as_rotation_xyz(const Degrees& angle_x, const Degrees& angle_y,
                           const Degrees& angle_z) {
    Quaternion rot(angle_x, angle_y, angle_z);
    return Mat4::as_rotation(rot);
}

Mat4 Mat4::as_scale(const smlt::Vec3& s) {
    Mat4 ret;
    shz_mat4x4_init_scale(&ret.m, s.x, s.y, s.z);
    return ret;
}

Mat4 Mat4::as_transform(const Vec3& t, const Quaternion& r, const Vec3& s) {
    Mat4 m;

    m[0] = (1.0f - 2.0f * (r.y * r.y + r.z * r.z)) * s.x;
    m[1] = (r.x * r.y + r.z * r.w) * s.x * 2.0f;
    m[2] = (r.x * r.z - r.y * r.w) * s.x * 2.0f;
    m[3] = 0.0f;
    m[4] = (r.x * r.y - r.z * r.w) * s.y * 2.0f;
    m[5] = (1.0f - 2.0f * (r.x * r.x + r.z * r.z)) * s.y;
    m[6] = (r.y * r.z + r.x * r.w) * s.y * 2.0f;
    m[7] = 0.0f;
    m[8] = (r.x * r.z + r.y * r.w) * s.z * 2.0f;
    m[9] = (r.y * r.z - r.x * r.w) * s.z * 2.0f;
    m[10] = (1.0f - 2.0f * (r.x * r.x + r.y * r.y)) * s.z;
    m[11] = 0.0f;
    m[12] = t.x;
    m[13] = t.y;
    m[14] = t.z;
    m[15] = 1.0f;

    return m;
}

Mat4 Mat4::as_translation(const Vec3& v) {
    Mat4 ret;
    ret[12] = v.x;
    ret[13] = v.y;
    ret[14] = v.z;
    ret[15] = 1.0f;

    return ret;
}

Mat4 Mat4::as_projection(const Degrees& fov, float aspect, float zNear,
                         float zFar) {
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

    auto fovy = Radians(fov).to_float();

    float const tanHalfFovy = std::tan(fovy * 0.5f);

    Mat4 result;

    result[0] = 1.0f / (aspect * tanHalfFovy);
    result[5] = 1.0f / (tanHalfFovy);
    result[10] = -(zFar + zNear) / (zFar - zNear);
    result[11] = -1.0f;
    result[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
    result[15] = 0.0f;

    return result;
}

Mat4 Mat4::as_orthographic(float left, float right, float bottom, float top,
                           float zNear, float zFar) {
    Mat4 result;

    result[0] = 2.0f / (right - left);
    result[5] = 2.0f / (top - bottom);
    result[10] = -2.0f / (zFar - zNear);

    result[12] = -(right + left) / (right - left);
    result[13] = -(top + bottom) / (top - bottom);
    result[14] = -(zFar + zNear) / (zFar - zNear);

    return result;
}

void Mat4::inverse() {
    Mat4 tmp;

    shz_mat4x4_inverse(&m, &tmp.m);
    shz_mat4x4_copy(&m, &tmp.m);
}

Plane Mat4::extract_plane(FrustumPlane plane) const {
    float t = 1.0f;
    Plane out;

    switch(plane) {
        case FRUSTUM_PLANE_RIGHT:
            out.n.x = m.elem[3] - m.elem[0];
            out.n.y = m.elem[7] - m.elem[4];
            out.n.z = m.elem[11] - m.elem[8];
            out.d = m.elem[15] - m.elem[12];
            break;
        case FRUSTUM_PLANE_LEFT:
            out.n.x = m.elem[3] + m.elem[0];
            out.n.y = m.elem[7] + m.elem[4];
            out.n.z = m.elem[11] + m.elem[8];
            out.d = m.elem[15] + m.elem[12];
            break;
        case FRUSTUM_PLANE_BOTTOM:
            out.n.x = m.elem[3] + m.elem[1];
            out.n.y = m.elem[7] + m.elem[5];
            out.n.z = m.elem[11] + m.elem[9];
            out.d = m.elem[15] + m.elem[13];
            break;
        case FRUSTUM_PLANE_TOP:
            out.n.x = m.elem[3] - m.elem[1];
            out.n.y = m.elem[7] - m.elem[5];
            out.n.z = m.elem[11] - m.elem[9];
            out.d = m.elem[15] - m.elem[13];
            break;
        case FRUSTUM_PLANE_FAR:
            out.n.x = m.elem[3] - m.elem[2];
            out.n.y = m.elem[7] - m.elem[6];
            out.n.z = m.elem[11] - m.elem[10];
            out.d = m.elem[15] - m.elem[14];
            break;
        case FRUSTUM_PLANE_NEAR:
            out.n.x = m.elem[3] + m.elem[2];
            out.n.y = m.elem[7] + m.elem[6];
            out.n.z = m.elem[11] + m.elem[10];
            out.d = m.elem[15] + m.elem[14];
            break;
        default:
            assert(0 && "Invalid plane index");
    }

    t = out.n.dot(out.n);

    const float inv_sqrt_t = fast_inverse_sqrt(t);
    out.n *= inv_sqrt_t;
    out.d *= inv_sqrt_t;

    return out;
}

Mat4 Mat4::as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
    /* If the up vector is parellel to the view vector then we swap the up Y/Z
     * axis */

    float d = up.dot((target - eye));
    auto tu = (almost_equal(d * d, 1.0f)) ? Vec3(up.x, up.z, up.y) : up;

    Vec3 f = (target - eye).normalized();
    Vec3 s = f.cross(tu).normalized();
    Vec3 u = s.cross(f);

    Mat4 ret;

    ret.m.elem[0] = s.x;
    ret.m.elem[1] = u.x;
    ret.m.elem[2] = -f.x;
    ret.m.elem[3] = 0.0;

    ret.m.elem[4] = s.y;
    ret.m.elem[5] = u.y;
    ret.m.elem[6] = -f.y;
    ret.m.elem[7] = 0.0;

    ret.m.elem[8] = s.z;
    ret.m.elem[9] = u.z;
    ret.m.elem[10] = -f.z;
    ret.m.elem[11] = 0.0;

    ret.m.elem[12] = -s.dot(eye);
    ret.m.elem[13] = -u.dot(eye);
    ret.m.elem[14] = f.dot(eye);
    ret.m.elem[15] = 1.0;

    return ret;
}

} // namespace smlt
