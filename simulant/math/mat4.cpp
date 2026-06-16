#include "mat4.h"
#include "../types.h"
#include "mat3.h"
#include "../deps/sh4zam/shz_matrix.h"
#include "quaternion.h"
#include "utils.h"

namespace smlt {

Mat4::Mat4() {
    shz_mat4x4_init_identity((shz_mat4x4_t*) data());
}

Mat4 Mat4::zero() {
    Mat4 r;
    shz_mat4x4_init_zero((shz_mat4x4_t*) r.data());
    return r;
}

Mat4 Mat4::operator*(const Mat4& rhs) const {
    Mat4 result;
    shz_mat4x4_mult((shz_mat4x4_t*) result.data(), (shz_mat4x4_t*)data(), (shz_mat4x4_t*)rhs.data());
    return result;
}

bool Mat4::operator==(const Mat4& rhs) const {
    return shz_mat4x4_equal((shz_mat4x4_t*)data(), (shz_mat4x4_t*)rhs.data());
}

Mat4 Mat4::as_rotation(const Quaternion& rhs) {
    Mat4 m;
    auto n = (shz_mat4x4_t*) m.data();
    shz_mat4x4_init_rotation_quat(n, (shz_quat_t&) rhs);
    return m;
}

Mat4 Mat4::inversed_transform() const {
    Mat4 result;

    auto n = (shz_mat4x4_t*) data();
    auto rn = (shz_mat4x4_t*) result.data();

    shz_mat4x4_inverse_block_triangular(n, rn);
    return result;
}

void Mat4::transpose() {
    auto n = (shz_mat4x4_t*) data();
    shz_mat4x4_transpose(n, n);
}


Vec4 Mat4::operator*(const Vec4& v) const {
    shz_vec4 vec;
    vec.x = v.x;
    vec.y = v.y;
    vec.z = v.z;
    vec.w = v.w;

    vec = shz_mat4x4_transform_vec4((shz_mat4x4_t*) data(), vec);
    return Vec4(vec.x, vec.y, vec.z, vec.w);
}

Vec3 Mat4::operator*(const Vec3& v) const {
    shz_vec3 vec;
    vec.x = v.x;
    vec.y = v.y;
    vec.z = v.z;

    vec = shz_mat4x4_transform_vec3((shz_mat4x4_t*) data(), vec);
    return Vec3(vec.x, vec.y, vec.z);
}

void Mat4::extract_rotation_and_translation(Quaternion& rotation,
                                            Vec3& translation) const {
    shz_vec3 trn, scale;
    shz_mat4x4_decompose((shz_mat4x4_t*) data(), &trn, (shz_quat_t*) &rotation, &scale);

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
    shz_mat4x4_init_scale((shz_mat4x4_t*) ret.data(), s.x, s.y, s.z);
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

    shz_mat4x4_inverse((shz_mat4x4_t*) data(), (shz_mat4x4_t*) tmp.data());
    shz_mat4x4_copy((shz_mat4x4_t*) data(), (shz_mat4x4_t*) tmp.data());
}

Plane Mat4::extract_plane(FrustumPlane plane) const {
    float t = 1.0f;
    Plane out;

    const float* elem = data();

    switch(plane) {
        case FRUSTUM_PLANE_RIGHT:
            out.n.x = elem[3] - elem[0];
            out.n.y = elem[7] - elem[4];
            out.n.z = elem[11] - elem[8];
            out.d = elem[15] - elem[12];
            break;
        case FRUSTUM_PLANE_LEFT:
            out.n.x = elem[3] + elem[0];
            out.n.y = elem[7] + elem[4];
            out.n.z = elem[11] + elem[8];
            out.d = elem[15] + elem[12];
            break;
        case FRUSTUM_PLANE_BOTTOM:
            out.n.x = elem[3] + elem[1];
            out.n.y = elem[7] + elem[5];
            out.n.z = elem[11] + elem[9];
            out.d = elem[15] + elem[13];
            break;
        case FRUSTUM_PLANE_TOP:
            out.n.x = elem[3] - elem[1];
            out.n.y = elem[7] - elem[5];
            out.n.z = elem[11] - elem[9];
            out.d = elem[15] - elem[13];
            break;
        case FRUSTUM_PLANE_FAR:
            out.n.x = elem[3] - elem[2];
            out.n.y = elem[7] - elem[6];
            out.n.z = elem[11] - elem[10];
            out.d = elem[15] - elem[14];
            break;
        case FRUSTUM_PLANE_NEAR:
            out.n.x = elem[3] + elem[2];
            out.n.y = elem[7] + elem[6];
            out.n.z = elem[11] + elem[10];
            out.d = elem[15] + elem[14];
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

    float* elem = ret.data();

    elem[0] = s.x;
    elem[1] = u.x;
    elem[2] = -f.x;
    elem[3] = 0.0;

    elem[4] = s.y;
    elem[5] = u.y;
    elem[6] = -f.y;
    elem[7] = 0.0;

    elem[8] = s.z;
    elem[9] = u.z;
    elem[10] = -f.z;
    elem[11] = 0.0;

    elem[12] = -s.dot(eye);
    elem[13] = -u.dot(eye);
    elem[14] = f.dot(eye);
    elem[15] = 1.0;

    return ret;
}

#ifndef NDEBUG
Mat4Scratch* Mat4Scratch::current_ = nullptr;
#endif

Mat4Scratch::Mat4Scratch(const Mat4& m) {
#ifndef NDEBUG
    assert(current_ == nullptr);
    current_ = this;
#endif
    shz_xmtrx_load_4x4((shz_mat4x4_t*) m.data());
}

Vec3 Mat4Scratch::transform_point(const Vec3& in) const {
    shz_vec3_t p = shz_xmtrx_transform_point3(shz_vec3_init(in.x, in.y, in.z));
    return Vec3(p.x, p.y, p.z);
}

Vec3 Mat4Scratch::transform_vector(const Vec3& in) const {
    shz_vec3_t r = shz_xmtrx_transform_vec3(shz_vec3_init(in.x, in.y, in.z));
    return Vec3(r.x, r.y, r.z);
}


} // namespace smlt
