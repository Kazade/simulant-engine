#pragma once

#include <array>
#include <cstdint>
#include <cstring>
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

struct Mat4 {
private:
    // We need some data that is *always* 8 byte aligned. Unfortunately
    // GCC on sh4 doesn't always do this on stack variables even if we specify
    // alignas! We work around this by adding 4-byte extra space and manually
    // aligning.
    alignas(8) uint8_t m[(sizeof(float) * 16) + 4];

public:
    Mat4();
    static Mat4 zero();

    Mat4(const FloatArray& arr) {
        std::copy(arr.begin(),
                  arr.begin() + std::min((unsigned)arr.size(), 16u), data());
    }

    Mat4 operator*(const Mat4& rhs) const;
    Vec4 operator*(const Vec4& rhs) const;
    Vec3 operator*(const Vec3& rhs) const;
    bool operator==(const Mat4& rhs) const;

    void extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) const;

    static Mat4 as_rotation_x(const Degrees& angle);
    static Mat4 as_rotation_y(const Degrees& angle);
    static Mat4 as_rotation_z(const Degrees& angle);
    static Mat4 as_rotation_xyz(const Degrees& angle_x, const Degrees& angle_y, const Degrees& angle_z);
    static Mat4 as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up);

    inline const float& operator[](const uint32_t index) const {
        return data()[index];
    }

    inline float& operator[](const uint32_t index){
        return data()[index];
    }

    inline const float& operator[](const int index) const {
        return data()[index];
    }

    inline float& operator[](const int index) {
        return data()[index];
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
        void* n = _native();
        return (float*) n;
    }

    const float* data() const {
        return const_cast<Mat4*>(this)->data();
    }

    void* _native() {
        return (void*) (((uintptr_t(m) + 7) & ~7));
    }

    const void* _native() const {
        return const_cast<Mat4*>(this)->_native();
    }

    // Faster inverse for TRS (translation/rotation/scale) matrices.
    // The world-space matrix of any actor is always a TRS matrix.
    Mat4 inversed_transform() const;
    void transpose();

    Mat4 transposed() const {
        auto cpy = *this;
        cpy.transpose();
        return cpy;
    }
};


/* A class used to optimise multiple operations using the same matrix. On
 * some platforms this uses special CPU instructions.
 *
 * Do not create multiple instances of this class at the same time.
 */
class Mat4Scratch {
public:
    Mat4Scratch(const Mat4& m);
#ifndef NDEBUG
    ~Mat4Scratch() {
        assert(current_ == this);
        current_ = nullptr;
    }
#endif

    Vec3 transform_point(const Vec3& in) const;
    Vec3 transform_vector(const Vec3& in) const;

private:
#ifndef NDEBUG
    static Mat4Scratch* current_;
#endif
};


}
