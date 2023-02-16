#pragma once

#include <array>
#include <cstring>
#include <cstdint>

namespace smlt {

struct Quaternion;
struct Vec3;
struct Vec4;
struct Plane;
struct Degrees;

enum FrustumPlane {
    FRUSTUM_PLANE_LEFT = 0,
    FRUSTUM_PLANE_RIGHT,
    FRUSTUM_PLANE_BOTTOM,
    FRUSTUM_PLANE_TOP,
    FRUSTUM_PLANE_NEAR,
    FRUSTUM_PLANE_FAR,
    FRUSTUM_PLANE_MAX
};

struct Mat4 {
private:
    float m[16];

public:

    Mat4() {
        memset(m, 0, sizeof(float) * 16);
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    Mat4(const Quaternion& rhs);
    Mat4(const Quaternion& rot, const Vec3& trans, const Vec3& scale);

    Mat4 operator*(const Mat4& rhs) const {

        Mat4 result;
        const float *m1 = &this->m[0], *m2 = &rhs.m[0];

        result.m[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
        result.m[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
        result.m[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
        result.m[3] = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];

        result.m[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
        result.m[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
        result.m[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
        result.m[7] = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];

        result.m[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
        result.m[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
        result.m[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
        result.m[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];

        result.m[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
        result.m[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
        result.m[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
        result.m[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];

        return result;
    }

    Vec4 operator*(const Vec4& rhs) const;
    Vec3 operator*(const Vec3& rhs) const;

    void extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) const;

    static Mat4 as_rotation_x(const Degrees& angle);
    static Mat4 as_rotation_y(const Degrees& angle);
    static Mat4 as_rotation_z(const Degrees& angle);
    static Mat4 as_rotation_xyz(const Degrees& angle_x, const Degrees& angle_y, const Degrees& angle_z);
    static Mat4 as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up);
    static Mat4 as_scaling(float s);
    static Mat4 from_pos_rot_scale(const Vec3& pos, const Quaternion& rot, const Vec3& scale);

    inline const float& operator[](const uint32_t index) const {
        return m[index];
    }

    inline float& operator[](const uint32_t index){
        return m[index];
    }

    inline const float& operator[](const int index) const {
        return m[index];
    }

    inline float& operator[](const int index) {
        return m[index];
    }

    static Mat4 as_translation(const Vec3& v);

    static Mat4 as_projection(const Degrees& fov, float aspect, float near, float far);

    static Mat4 as_orthographic(float left, float right, float bottom, float top, float zNear, float zFar);

    void inverse();

    Mat4 inversed() const {
        Mat4 ret = *this;
        ret.inverse();
        return ret;
    }

    Plane extract_plane(FrustumPlane plane) const;

    const float* data() const {
        return &m[0];
    }

};


}
