#pragma once

#include <array>
#include <cstdint>

namespace smlt {

struct Mat4;
struct Degrees;
struct Vec3;
struct Quaternion;

struct Mat3 {
private:
    std::array<float, 9> m;

public:
    static Mat3 from_rotation_x(const Degrees& angle);
    static Mat3 from_rotation_y(const Degrees& angle);
    static Mat3 from_rotation_z(const Degrees& angle);

    Mat3() {
        m.fill(0);
        m[0] = m[4] = m[8] = 1.0f;
    }

    Mat3(const float* data) {
        for(std::size_t i = 0; i < 9; ++i) {
            m[i] = data[i];
        }
    }

    Mat3(const Mat4& rhs);
    Mat3(const Quaternion& q);

    const float& operator[](const uint32_t index) const {
        return m[index];
    }

    const float& operator[](const int index) const{
        return m[index];
    }

    float& operator[](const uint32_t index) {
        return m[index];
    }

    float& operator[](const int index) {
        return m[index];
    }

    Vec3 transform_vector(const Vec3& v) const;

    const float* data() const {
        return &m[0];
    }

    void inverse();

    Mat3 inversed() const {
        Mat3 ret = *this;
        ret.inverse();
        return ret;
    }

    void transpose() {
        std::swap(m[1], m[3]);
        std::swap(m[2], m[6]);
        std::swap(m[5], m[7]);
    }

    Mat3 transposed() const {
        Mat3 ret = *this;
        ret.transpose();
        return ret;
    }

    float determinant() const;
    Mat3 adjugate() const;

    Mat3& operator*=(const float rhs) {
        for(std::size_t i = 0; i < 9; ++i) {
            m[i] *= rhs;
        }

        return *this;
    }

    Mat3 operator*(const float rhs) const {
        return Mat3(*this) *= rhs;
    }

};


}
