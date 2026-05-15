#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include "../deps/sh4zam/shz_matrix.h"
#include <vector>
#include <stdio.h>

#if defined(_MSC_VER)
#include "degrees.h"
#endif

namespace smlt {

struct Quaternion;
struct Vec3;
struct Vec4;
struct Plane;
#if !defined(_MSC_VER)
struct Degrees;
#endif

enum FrustumPlane {
    FRUSTUM_PLANE_LEFT = 0,
    FRUSTUM_PLANE_RIGHT,
    FRUSTUM_PLANE_BOTTOM,
    FRUSTUM_PLANE_TOP,
    FRUSTUM_PLANE_NEAR,
    FRUSTUM_PLANE_FAR,
    FRUSTUM_PLANE_MAX
};

typedef std::vector<float> FloatArray;

#include <assert.h>

struct alignas(8) Mat4 {
private:
    uint8_t m[sizeof(shz_mat4x4_t) + 4];

public:
    Mat4() {
        shz_mat4x4_init_identity(native());
    }

    static Mat4 zero() {
        Mat4 r;
        shz_mat4x4_init_zero(r.native());
        return r;
    }

    Mat4(const FloatArray& arr) {
        std::copy(arr.begin(),
                  arr.begin() + std::min((unsigned)arr.size(), 16u), native()->elem);
    }

    Mat4 operator*(const Mat4& rhs) const {
        Mat4 result;
        shz_mat4x4_mult(result.native(), native(), rhs.native());
        return result;
    }

    Vec4 operator*(const Vec4& rhs) const;
    Vec3 operator*(const Vec3& rhs) const;

    bool operator==(const Mat4& rhs) const {
        return shz_mat4x4_equal(native(), rhs.native());
    }

    void extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) const;

    static Mat4 as_rotation_x(const Degrees& angle);
    static Mat4 as_rotation_y(const Degrees& angle);
    static Mat4 as_rotation_z(const Degrees& angle);
    static Mat4 as_rotation_xyz(const Degrees& angle_x, const Degrees& angle_y, const Degrees& angle_z);
    static Mat4 as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up);

    inline const float& operator[](const uint32_t index) const {
        return native()->elem[index];
    }

    inline float& operator[](const uint32_t index){
        return native()->elem[index];
    }

    inline const float& operator[](const int index) const {
        return native()->elem[index];
    }

    inline float& operator[](const int index) {
        return native()->elem[index];
    }

    static Mat4 as_translation(const Vec3& v);
    static Mat4 as_rotation(const Quaternion& r);
    static Mat4 as_scale(const Vec3& v);
    static Mat4 as_transform(const Vec3& t, const Quaternion& r, const Vec3& s);

    static Mat4 as_projection(const Degrees& fov, float aspect, float near, float far);

    static Mat4 as_orthographic(float left, float right, float bottom, float top, float zNear, float zFar);

    void inverse();

    Mat4 inversed() const {
        Mat4 ret = *this;
        ret.inverse();
        return ret;
    }

    Plane extract_plane(FrustumPlane plane) const;

    float* data() {
        return &native()->elem[0];
    }

    const float* data() const {
        return &native()->elem[0];
    }

    shz_mat4x4_t* native() {
        return (shz_mat4x4_t*)(((uintptr_t(m) + 7) & ~7));
    }

    const shz_mat4x4_t* native() const {
        return const_cast<Mat4*>(this)->native();
    }

    // Faster inverse for TRS (translation/rotation/scale) matrices.
    // The world-space matrix of any actor is always a TRS matrix.
    Mat4 inversed_transform() const {
        Mat4 result;
        shz_mat4x4_inverse_block_triangular(native(), result.native());
        return result;
    }

    void transpose() {
        shz_mat4x4_transpose(native(), native());
    }

    Mat4 transposed() const {
        auto cpy = *this;
        cpy.transpose();
        return cpy;
    }
};


}
