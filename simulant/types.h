/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <iostream>
#include <memory>
#include <tuple>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "colour.h"
#include "generic/optional.h"

#include "math/vec2.h"

#include "deps/glm/vec3.hpp"
#include "deps/glm/vec4.hpp"
#include "deps/glm/mat4x4.hpp"
#include "deps/glm/mat3x3.hpp"
#include "deps/glm/gtc/quaternion.hpp"
#include "deps/glm/gtx/norm.hpp"
#include "deps/glm/gtx/compatibility.hpp"
#include "deps/glm/gtx/intersect.hpp"
#include "deps/glm/gtc/epsilon.hpp"
#include "deps/glm/gtx/transform.hpp"
#include "deps/glm/gtc/matrix_transform.hpp"
#include "deps/glm/gtx/matrix_decompose.hpp"
#include "deps/glm/gtc/type_ptr.hpp"
#include "deps/glm/gtx/rotate_vector.hpp"

#include "generic/manager.h"
#include "generic/auto_weakptr.h"
#include "generic/unique_id.h"
#include "utils/unicode.h"
#include "material_constants.h"

#define DEFINE_SIGNAL(prototype, name) \
    public: \
        prototype& name() { return name##_; } \
    private: \
        prototype name##_;


namespace smlt {

struct Vec3;
struct Mat3;
struct Quaternion;

struct Radians;
struct Degrees {
    Degrees():
        value(0) {}

    explicit Degrees(float value):
        value(value) {}

    Degrees(const Radians& rhs);

    float value;

    Degrees operator-() const {
        Degrees ret = *this;
        ret.value = -ret.value;
        return ret;
    }

    bool operator==(const Degrees& rhs) const {
        return value == rhs.value;
    }

    bool operator!=(const Degrees& rhs) const {
        return !(*this == rhs);
    }

    bool is_effectively_equal_to(const Degrees& rhs, float epsilon=0.0f) {
        // Returns equal if the values represent basically the same thing (e.g. -90 == 270)
        float rhs_v = rhs.value;
        if(rhs_v < 0) rhs_v += 360.0f;

        float lhs_v = value;
        if(lhs_v < 0) lhs_v += 360.0f;

        return lhs_v - epsilon < rhs_v && lhs_v + epsilon > rhs_v;
    }
};

struct Radians {
    Radians():
        value(0) {}

    explicit Radians(float value):
        value(value) {}

    Radians(const Degrees& rhs);

    float value;
};

Radians to_radians(const Degrees& degrees);
Degrees to_degrees(const Radians& radians);

struct Euler {
    Euler(float x, float y, float z):
        x(x), y(y), z(z) {}

    Degrees x;
    Degrees y;
    Degrees z;
};

struct Plane;
struct Vec4;

enum FrustumPlane {
    FRUSTUM_PLANE_LEFT = 0,
    FRUSTUM_PLANE_RIGHT,
    FRUSTUM_PLANE_BOTTOM,
    FRUSTUM_PLANE_TOP,
    FRUSTUM_PLANE_NEAR,
    FRUSTUM_PLANE_FAR,
    FRUSTUM_PLANE_MAX
};

template<typename T>
bool almost_equal(const T& lhs, const T& rhs) {
    return lhs + std::numeric_limits<T>::epsilon() > rhs &&
           lhs - std::numeric_limits<T>::epsilon() < rhs;
}

struct Mat4 : private glm::mat4x4 {
private:
    Mat4(const glm::mat4x4& rhs) {
        *this = rhs;
    }

    Mat4& operator=(const glm::mat4x4& rhs) {
        glm::mat4::operator =(rhs);
        return *this;
    }

    friend struct Vec4;
    friend struct Quaternion;
    friend struct Mat3;
    friend struct Vec3;
public:
#ifndef __clang__
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)
    using glm::mat4x4::value_type;
#endif
#endif

    Mat4() {
        glm::mat4x4();
    }

    Mat4(const Quaternion& rhs) {
        *this = glm::mat4_cast((const glm::quat&) rhs);
    }

    Mat4 operator*(const Mat4& rhs) const {
        Mat4 result = glm::operator *(*this, rhs);
        return result;
    }

    Vec4 operator*(const Vec4& rhs) const;

    void extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) const;

    static Mat4 as_rotation_x(const Degrees& angle);
    static Mat4 as_rotation_y(const Degrees& angle);
    static Mat4 as_rotation_z(const Degrees& angle);
    static Mat4 as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up);
    static Mat4 as_scaling(float s);

    const float& operator[](const uint32_t index) const {
        uint32_t col = index / 4;
        uint32_t row = index % 4;

        const glm::mat4x4& self = *this;
        return self[col][row];
    }

    float& operator[](const uint32_t index){
        uint32_t col = index / 4;
        uint32_t row = index % 4;

        glm::mat4x4& self = *this;
        return self[col][row];
    }

    static Mat4 as_translation(const Vec3& v);

    static Mat4 as_projection(const Degrees& fov, float aspect, float near, float far) {
        Mat4 ret;
        ret = glm::perspective(Radians(fov).value, aspect, near, far);
        return ret;
    }

    static Mat4 as_orthographic(float left, float right, float bottom, float top, float near, float far) {
        Mat4 ret;
        ret = glm::ortho(left, right, bottom, top, near, far);
        return ret;
    }

    void inverse() {
        *this = glm::inverse((glm::mat4x4) *this);
    }

    Mat4 inversed() const {
        Mat4 ret = *this;
        ret.inverse();
        return ret;
    }

    Plane extract_plane(FrustumPlane plane) const;

    const float* data() const {
        return glm::value_ptr(*this);
    }

};

struct Mat3 : private glm::mat3x3 {
private:
    Mat3(const glm::mat3x3& rhs) {
        *this = rhs;
    }

    Mat3& operator=(const glm::mat3x3& rhs) {
        glm::mat3x3::operator =(rhs);
        return *this;
    }

public:
#ifndef __clang__
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)
    using glm::mat3x3::value_type;
#endif
#endif

    static Mat3 from_rotation_x(const Degrees& angle);
    static Mat3 from_rotation_y(const Degrees& angle);
    static Mat3 from_rotation_z(const Degrees& angle);

    Mat3() {
        glm::mat3x3();
    }

    Mat3(const float* data) {
        for(uint32_t i = 0; i < 9; ++i) {
            (*this)[i] = data[i];
        }
    }

    Mat3(const Mat4& rhs);

    const float operator[](const uint32_t index) const {
        uint32_t col = index / 3;
        auto& column = glm::mat3x3::operator [](col);
        return column[index - (3 * col)];
    }

    float& operator[](const uint32_t index) {
        uint32_t col = index / 3;
        auto& column = glm::mat3x3::operator [](col);
        return column[index - (3 * col)];
    }

    Vec3 transform_vector(const Vec3& v) const;

    const float* data() const {
        return glm::value_ptr(*this);
    }

    void inverse() {
        *this = glm::inverse((const glm::mat3x3&) *this);
    }

    Mat3 inversed() const {
        Mat3 ret = *this;
        ret.inverse();
        return ret;
    }

    void transpose() {
        *this = glm::transpose((const glm::mat3x3&) *this);
    }

    Mat3 transposed() const {
        Mat3 ret = *this;
        ret.transpose();
        return ret;
    }
};



struct Degrees;

struct Vec3 : private glm::vec3 {
private:
    Vec3(const glm::vec3& rhs) {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
    }

    Vec3& operator=(const glm::vec3& rhs) {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;

        return *this;
    }

    friend struct Quaternion;
    friend struct Mat4;
    friend struct Mat3;
    friend struct Vec2;
    friend struct Ray;

public:
    using glm::vec3::value_type;

    static const Vec3 NEGATIVE_X;
    static const Vec3 POSITIVE_X;
    static const Vec3 NEGATIVE_Y;
    static const Vec3 POSITIVE_Y;
    static const Vec3 POSITIVE_Z;
    static const Vec3 NEGATIVE_Z;

    using glm::vec3::x;
    using glm::vec3::y;
    using glm::vec3::z;

    Vec3() {
        x = y = z = 0;
    }

    Vec3(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    Vec3(const Vec2& v2, float z) {
        this->x = v2.x;
        this->y = v2.y;
        this->z = z;
    }

    Vec3(const Vec3& v) {
        this->x = v.x;
        this->y = v.y;
        this->z = v.z;
    }

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vec3& operator+=(const Vec3& rhs) {
        *this = *this + rhs;
        return *this;
    }

    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3 operator*(const Vec3& rhs) const {
        return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    Vec3 operator*(float rhs) const {
        Vec3 result(x * rhs, y * rhs, z * rhs);
        return result;
    }

    Vec3 operator*(const Quaternion& rhs) const;
    Vec3& operator*=(const Quaternion& rhs) {
        *this = *this * rhs;
        return *this;
    }

    Vec3& operator*=(float rhs) {
        *this = *this * rhs;
        return *this;
    }

    Vec3& operator/=(float rhs) {
        *this = *this / rhs;
        return *this;
    }

    Vec3 operator/(float rhs) const {
        Vec3 result(x / rhs, y / rhs, z / rhs);
        return result;
    }

    bool operator==(const Vec3& rhs) const {
        return glm::operator==(*this, rhs);
    }

    bool operator!=(const Vec3& rhs) const {
        return !(*this == rhs);
    }

    void set(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    float length() const {        
        return glm::length((glm::vec3) *this);
    }

    float length_squared() const {
        return glm::length2((glm::vec3) *this);
    }

    const smlt::Vec3 normalized() const {
        smlt::Vec3 result = glm::normalize((glm::vec3) *this);
        return result;
    }

    void normalize() {
        *this = glm::normalize((glm::vec3) *this);
    }

    Vec3 lerp(const Vec3& end, float t) {
        if(t > 1.0f) t = 1.0f;
        if(t < 0.0f) t = 0.0f;

        Vec3 ret;
        ret = glm::lerp((glm::vec3) *this, end, t);
        return ret;
    }

    Vec3 rotated_by(const Quaternion& q) const {
        return glm::rotate((const glm::quat&) q, (const glm::vec3&) *this);
    }

    Vec3 rotated_by(const Mat3& rot) const {
        return rot.transform_vector(*this);
    }

    /* Note: We do *not* supply multiply operators with Mat4 because
     * the behaviour you want depends on whether want translation or not
     * (e.g. is this a point in space or a vector). Instead we provide
     * rotated_by() and transformed_by() the former assumes a W component of
     * 0 whereas the latter assumes a W component of 1.0
     */

    Vec3 rotated_by(const Mat4& rot) const;

    Vec3 transformed_by(const Mat4& trans) const;

    Vec3 project_onto_vec3(const Vec3& projection_target) const {
        Vec3 unitW = this->normalized();
        Vec3 unitV = projection_target.normalized();

        float cos0 = unitW.dot(unitV);

        unitV *= (this->length() * cos0);

        return unitV;
    }

    float dot(const smlt::Vec3& rhs) const {
        return glm::dot((glm::vec3) *this, (glm::vec3) rhs);
    }

    smlt::Vec3 cross(const smlt::Vec3& rhs) const {
        return Vec3(glm::cross((glm::vec3) *this, (glm::vec3) rhs));
    }

    smlt::Vec3 limit(float l) {
        if(length() > l) {
            normalize();
            *this *= l;
        }

        return *this;
    }

    template<typename Container>
    static Vec3 find_average(const Container& vectors) {
        Vec3 ret;
        for(auto& v: vectors) {
            ret += v;
        }

        ret /= float(vectors.size());
        return ret;
    }

    static float distance(const smlt::Vec3& lhs, const smlt::Vec3& rhs) {
        return (rhs - lhs).length();
    }


    inline Vec3 parallel_component(const Vec3& unit_basis) const {
        const float projection = this->dot(unit_basis);
        return unit_basis * projection;
    }

    // return component of vector perpendicular to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))

    inline Vec3 perpendicular_component (const Vec3& unit_basis) const {
        return (*this) - parallel_component(unit_basis);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vec3& vec);

    unicode to_string() const {
        return _u("({0},{1},{2})").format(x, y, z);
    }

    Vec3 perpendicular() const;
    Vec3 random_deviant(const Degrees& angle, const Vec3 up=Vec3()) const;
};

struct AxisAngle {
    Vec3 axis;
    Degrees angle;
};

struct Quaternion : private glm::quat {
private:
    Quaternion(const glm::quat& rhs) {
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        this->w = rhs.w;
    }

    Quaternion& operator=(const glm::quat& rhs) {
        glm::quat::operator =(rhs);
        return *this;
    }

    friend struct Vec3;
    friend struct Mat4;
    friend struct Mat3;
public:
    using glm::quat::x;
    using glm::quat::y;
    using glm::quat::z;
    using glm::quat::w;

    Quaternion() {
        x = y = z = 0;
        w = 1;
    }

    Quaternion(Degrees pitch, Degrees yaw, Degrees roll);

    Quaternion(const Vec3& axis, const Degrees& degrees);
    Quaternion(const Mat3& rot_matrix);

    Quaternion(float x, float y, float z, float w) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    Vec3 rotate_vector(const Vec3& v) const;

    Euler to_euler() const;

    AxisAngle to_axis_angle() const;

    void normalize() {
        *this = glm::normalize((glm::quat) *this);
    }

    const Quaternion normalized() {
        Quaternion result = *this;
        result.normalize();
        return result;
    }

    void inverse() {
        glm::quat::operator =(glm::inverse(*this));
    }

    const Quaternion inversed() const {
        Quaternion result(*this);
        result.inverse();
        return result;
    }

    bool operator==(const Quaternion& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

    bool operator!=(const Quaternion& rhs) const {
        return !(*this == rhs);
    }

    Quaternion operator*(const Quaternion& rhs) const {
        glm::quat l = *this;
        glm::quat r = rhs;

        return Quaternion(l * r);
    }

    Quaternion slerp(const Quaternion& rhs, float t) {
        if(t > 1.0f) t = 1.0f;
        if(t < 0.0f) t = 0.0f;

        Quaternion result;
        result = glm::slerp(*this, rhs, t);
        return result;
    }

    const Degrees pitch() const {
        return Radians(glm::pitch(*this));
    }

    const Degrees yaw() const {
        return Radians(glm::yaw(*this));
    }

    const Degrees roll() const {
        return Radians(glm::roll(*this));
    }

    Vec3 forward() const {
        // OpenGL coordinate system has Neg-z as "forward"
        return Vec3::NEGATIVE_Z.rotated_by(*this);
    }

    Vec3 up() const {
        return Vec3::POSITIVE_Y.rotated_by(*this);
    }

    Vec3 right() const {
        return Vec3::POSITIVE_X.rotated_by(*this);
    }

    static Quaternion as_look_at(const Vec3& direction, const Vec3& up);
};

class PlaneClassification {
private:
    friend struct Plane;
    PlaneClassification(int8_t v):
        value_(v) {}

    int8_t value_;
public:
    bool is_behind_plane() const { return value_ == -1; }
    bool is_on_plane() const { return value_ == 0; }
    bool is_in_front_of_plane() const { return value_ == 1; }
};

struct Plane {
    Vec3 n;
    float d;

    Plane():
        n(Vec3()),
        d(0) {
    }

    Plane(const Vec3& N, float D):
        n(N),
        d(D) {

    }

    Plane(float A, float B, float C, float D):
        n(A, B, C),
        d(D) {

    }

    Vec3 project(const Vec3& p);

    Vec3 normal() const {
        return n;
    }

    float distance_to(const Vec3& p);

    PlaneClassification classify_point(const Vec3& p) const;

    static smlt::optional<Vec3> intersect_planes(
        const Plane& p1,
        const Plane& p2,
        const Plane& p3
    );
};


struct Vec4 : private glm::vec4 {
private:
    Vec4(const glm::vec4& rhs) {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
        w = rhs.w;
    }

    Vec4& operator=(const glm::vec4& rhs) {
        glm::vec4::operator =(rhs);
        return *this;
    }

    friend struct Vec3;
    friend struct Mat4;
public:
    using glm::vec4::x;
    using glm::vec4::y;
    using glm::vec4::z;
    using glm::vec4::w;

    Vec4() {
        x = y = z = 0;
    }

    Vec4(float x, float y, float z, float w) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    Vec4(const Vec3& v, float w) {
        this->x = v.x;
        this->y = v.y;
        this->z = v.z;
        this->w = w;
    }

    bool operator==(const Vec4& rhs) const {
        return glm::operator==(*this, rhs);
    }

    bool operator!=(const Vec4& rhs) const {
        return !(*this == rhs);
    }

    Vec4 operator-(const Vec4& rhs) const {
        return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }

    Vec4 operator*(const float& rhs) const {
        return Vec4(
            x * rhs,
            y * rhs,
            z * rhs,
            w * rhs
        );
    }

    void normalize() {
        *this = glm::normalize((glm::vec4) *this);
    }

    const smlt::Vec4 normalized() const {
        smlt::Vec4 result = *this;
        result.normalize();
        return result;
    }

    Vec4 operator+(const Vec4& rhs) const {
        return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }
};

struct AABB;

struct Ray {
    Vec3 start;
    Vec3 dir;
    Vec3 dir_inv;

    Ray() = default;

    Ray(const Vec3& start, const Vec3& dir):
        start(start),
        dir(dir) {

        dir_inv = Vec3(1.0 / dir.x, 1.0f / dir.y, 1.0 / dir.z);
    }

    bool intersects_aabb(const AABB& aabb) const;

    bool intersects_triangle(
        const Vec3& v1, const Vec3& v2, const Vec3& v3,
        Vec3* intersection=nullptr, Vec3* normal=nullptr, float* distance=nullptr
    ) const;
};

class AABB {
    /* This was originally a basic struct but for performance reasons it's now a class
     * so that we can store things like pre-calculate corners and know they are kept up-to-date
     * with setters */

    Vec3 min_;
    Vec3 max_;

    std::array<Vec3, 8> corners_;

    void rebuild_corners() {
        corners_[0] = Vec3(min_.x, min_.y, min_.z);
        corners_[1] = Vec3(max_.x, min_.y, min_.z);
        corners_[2] = Vec3(max_.x, min_.y, max_.z);
        corners_[3] = Vec3(min_.x, min_.y, max_.z);

        corners_[4] = Vec3(min_.x, max_.y, min_.z);
        corners_[5] = Vec3(max_.x, max_.y, min_.z);
        corners_[6] = Vec3(max_.x, max_.y, max_.z);
        corners_[7] = Vec3(min_.x, max_.y, max_.z);
    }

public:
    void set_min(const Vec3& min) {
        if(min_ != min) {
            min_ = min;
            rebuild_corners();
        }
    }

    void set_min_x(float x) {
        if(x != min_.x) {
            min_.x = x;
            rebuild_corners();
        }
    }

    void set_min_y(float y) {
        if(y != min_.y) {
            min_.y = y;
            rebuild_corners();
        }
    }

    void set_min_z(float z) {
        if(z != min_.z) {
            min_.z = z;
            rebuild_corners();
        }
    }


    void set_max_x(float x) {
        if(x != max_.x) {
            max_.x = x;
            rebuild_corners();
        }
    }

    void set_max_y(float y) {
        if(y != max_.y) {
            max_.y = y;
            rebuild_corners();
        }
    }

    void set_max_z(float z) {
        if(z != max_.z) {
            max_.z = z;
            rebuild_corners();
        }
    }

    void set_max(const Vec3& max) {
        if(max_ != max) {
            max_ = max;
            rebuild_corners();
        }
    }

    const Vec3& min() const { return min_; }
    const Vec3& max() const { return max_; }

    AABB() {
        rebuild_corners();
    }

    AABB(const Vec3& min, const Vec3& max);

    AABB(const Vec3& centre, float width);

    AABB(const Vec3& centre, float xsize, float ysize, float zsize);

    AABB(const Vec3* vertices, const std::size_t count);

    const float width() const {
        return fabs(max_.x - min_.x);
    }

    const float height() const {
        return fabs(max_.y - min_.y);
    }

    const float depth() const  {
        return fabs(max_.z - min_.z);
    }

    const Vec3 dimensions() const {
        return Vec3(width(), height(), depth());
    }

    const float max_dimension() const {
        return std::max(width(), std::max(height(), depth()));
    }

    bool intersects(const AABB& other) const {
        auto acx = (min_.x + max_.x) * 0.5;
        auto acy = (min_.y + max_.y) * 0.5;
        auto acz = (min_.z + max_.z) * 0.5;

        auto bcx = (other.min_.x + other.max_.x) * 0.5;
        auto bcy = (other.min_.y + other.max_.y) * 0.5;
        auto bcz = (other.min_.z + other.max_.z) * 0.5;

        auto arx = (max_.x - min_.x) * 0.5;
        auto ary = (max_.y - min_.y) * 0.5;
        auto arz = (max_.z - min_.z) * 0.5;

        auto brx = (other.max_.x - other.min_.x) * 0.5;
        auto bry = (other.max_.y - other.min_.y) * 0.5;
        auto brz = (other.max_.z - other.min_.z) * 0.5;

        bool x = fabs(acx - bcx) <= (arx + brx);
        bool y = fabs(acy - bcy) <= (ary + bry);
        bool z = fabs(acz - bcz) <= (arz + brz);

        return x && y && z;
    }

    Vec3 centre() const {
        return Vec3(min_) + ((Vec3(max_) - Vec3(min_)) * 0.5f);
    }

    const bool has_zero_area() const {
        /*
         * Returns True if the AABB has two or more zero dimensions
         */
        bool empty_x = width() == 0.0f;
        bool empty_y = height() == 0.0f;
        bool empty_z = depth() == 0.0f;

        return (empty_x && empty_y) || (empty_x && empty_z) || (empty_y && empty_z);
    }

    bool contains_point(const Vec3& p) const {
        if(p.x >= min_.x && p.x <= max_.x &&
           p.y >= min_.y && p.y <= max_.y &&
           p.z >= min_.z && p.z <= max_.z) {
            return true;
        }

        return false;
    }

    const std::array<Vec3, 8>& corners() const {
        return corners_;
    }
};

std::ostream& operator<<(std::ostream& stream, const Vec2& vec);
std::ostream& operator<<(std::ostream& stream, const Vec3& vec);
std::ostream& operator<<(std::ostream& stream, const Vec4& vec);
std::ostream& operator<<(std::ostream& stream, const Quaternion& quat);

smlt::Vec2 operator*(float lhs, const smlt::Vec2& rhs);

smlt::Vec3 operator*(float lhs, const smlt::Vec3& rhs);
smlt::Vec3 operator/(float lhs, const smlt::Vec3& rhs);
smlt::Vec3 operator-(const smlt::Vec3& vec);
smlt::Quaternion operator-(const smlt::Quaternion& q);



namespace math {

float lerp(float a, float b, float t);
Degrees lerp_angle(Degrees a, Degrees b, float t);
Radians lerp_angle(Radians a, Radians b, float t);

}


const float PI = glm::pi<float>();
const float TWO_PI = PI * 2.0f;
const float PI_OVER_180 = PI / 180.0f;
const float PI_UNDER_180 = 180.0f / PI;

enum VertexAttribute {
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_4F
};

struct VertexSpecification {
    static const VertexSpecification DEFAULT;
    static const VertexSpecification POSITION_ONLY;
    static const VertexSpecification POSITION_AND_DIFFUSE;

    VertexAttribute position_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute normal_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord0_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord1_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord2_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord3_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord4_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord5_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord6_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord7_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute diffuse_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute specular_attribute = VERTEX_ATTRIBUTE_NONE;

    VertexSpecification() = default;
    VertexSpecification(
        VertexAttribute position,
        VertexAttribute normal = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord0 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord1 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord2 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord3 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord4 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord5 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord6 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord7 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute diffuse = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute specular = VERTEX_ATTRIBUTE_NONE
    );

    bool operator==(const VertexSpecification& rhs) const {
        return position_attribute == rhs.position_attribute &&
               normal_attribute == rhs.normal_attribute  &&
                texcoord0_attribute == rhs.texcoord0_attribute &&
                texcoord1_attribute == rhs.texcoord1_attribute &&
                texcoord2_attribute == rhs.texcoord2_attribute &&
                texcoord3_attribute == rhs.texcoord3_attribute &&
                texcoord4_attribute == rhs.texcoord4_attribute &&
                texcoord5_attribute == rhs.texcoord5_attribute &&
                texcoord6_attribute == rhs.texcoord6_attribute &&
                texcoord7_attribute == rhs.texcoord7_attribute &&
                diffuse_attribute == rhs.diffuse_attribute &&
                specular_attribute == rhs.specular_attribute;
    }

    bool operator!=(const VertexSpecification& rhs) const {
        return !(*this == rhs);
    }

    inline uint32_t stride() const { return stride_; }

    bool has_positions() const { return bool(position_attribute); }
    bool has_normals() const { return bool(normal_attribute); }

    bool has_texcoordX(uint8_t which) const;

    const VertexAttribute texcoordX_attribute(uint8_t which) const;

    bool has_texcoord0() const { return bool(texcoord0_attribute); }
    bool has_texcoord1() const { return bool(texcoord1_attribute); }
    bool has_texcoord2() const { return bool(texcoord2_attribute); }
    bool has_texcoord3() const { return bool(texcoord3_attribute); }
    bool has_texcoord4() const { return bool(texcoord4_attribute); }
    bool has_texcoord5() const { return bool(texcoord5_attribute); }
    bool has_texcoord6() const { return bool(texcoord6_attribute); }
    bool has_texcoord7() const { return bool(texcoord7_attribute); }

    bool has_diffuse() const { return bool(diffuse_attribute); }
    bool has_specular() const { return bool(specular_attribute); }    

    uint32_t position_offset(bool check=true) const;
    uint32_t normal_offset(bool check=true) const;
    uint32_t texcoord0_offset(bool check=true) const;
    uint32_t texcoord1_offset(bool check=true) const;
    uint32_t texcoord2_offset(bool check=true) const;
    uint32_t texcoord3_offset(bool check=true) const;
    uint32_t texcoord4_offset(bool check=true) const;
    uint32_t texcoord5_offset(bool check=true) const;
    uint32_t texcoord6_offset(bool check=true) const;
    uint32_t texcoord7_offset(bool check=true) const;

    uint32_t texcoordX_offset(uint8_t which, bool check=true) const;

    uint32_t diffuse_offset(bool check=true) const;
    uint32_t specular_offset(bool check=true) const;

    void recalc_stride();

private:
    uint32_t stride_;
};

enum IndexType {
    INDEX_TYPE_8_BIT,
    INDEX_TYPE_16_BIT,
    INDEX_TYPE_32_BIT
};

enum BlendType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOUR,
    BLEND_ALPHA,
    BLEND_ONE_ONE_MINUS_ALPHA
};

enum MeshArrangement {
    MESH_ARRANGEMENT_POINTS,
    MESH_ARRANGEMENT_TRIANGLES,
    MESH_ARRANGEMENT_TRIANGLE_FAN,
    MESH_ARRANGEMENT_TRIANGLE_STRIP,
    MESH_ARRANGEMENT_LINES,
    MESH_ARRANGEMENT_LINE_STRIP
};

enum AvailablePartitioner {
    PARTITIONER_NULL,
    PARTITIONER_HASH
};

enum LightType {
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_SPOT_LIGHT
};

enum AspectRatio {
    ASPECT_RATIO_CUSTOM,
    ASPECT_RATIO_4_BY_3,
    ASPECT_RATIO_16_BY_9,
    ASPECT_RATIO_16_BY_10
};

typedef float Ratio; //FIXME: Custom type to restrict between 0 and 1

enum ProjectionType {
    PROJECTION_TYPE_PERSPECTIVE,
    PROJECTION_TYPE_ORTHOGRAPHIC
};

enum BufferClearFlag {
    BUFFER_CLEAR_COLOUR_BUFFER = 0x1,
    BUFFER_CLEAR_DEPTH_BUFFER = 0x2,
    BUFFER_CLEAR_STENCIL_BUFFER = 0x4,
    BUFFER_CLEAR_ALL = BUFFER_CLEAR_COLOUR_BUFFER | BUFFER_CLEAR_DEPTH_BUFFER | BUFFER_CLEAR_STENCIL_BUFFER
};

typedef int32_t RenderPriority;
const RenderPriority RENDER_PRIORITY_ABSOLUTE_BACKGROUND = -250;
const RenderPriority RENDER_PRIORITY_BACKGROUND = -100;
const RenderPriority RENDER_PRIORITY_DISTANT = -50;
const RenderPriority RENDER_PRIORITY_MAIN = 0;
const RenderPriority RENDER_PRIORITY_NEAR = 50;
const RenderPriority RENDER_PRIORITY_FOREGROUND = 100;
const RenderPriority RENDER_PRIORITY_ABSOLUTE_FOREGROUND = 250;
const RenderPriority RENDER_PRIORITY_MAX = RENDER_PRIORITY_ABSOLUTE_FOREGROUND + 1;

const std::vector<RenderPriority> RENDER_PRIORITIES = {
    RENDER_PRIORITY_ABSOLUTE_BACKGROUND,
    RENDER_PRIORITY_BACKGROUND,
    RENDER_PRIORITY_DISTANT,
    RENDER_PRIORITY_MAIN,
    RENDER_PRIORITY_NEAR,
    RENDER_PRIORITY_FOREGROUND,
    RENDER_PRIORITY_ABSOLUTE_FOREGROUND
};

enum LoggingLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
};

enum PolygonMode {
    POLYGON_MODE_FILL,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT
};

enum RenderableCullingMode {
    RENDERABLE_CULLING_MODE_NEVER,
    RENDERABLE_CULLING_MODE_PARTITIONER
};

enum CullMode {
    CULL_MODE_NONE,
    CULL_MODE_BACK_FACE,
    CULL_MODE_FRONT_FACE,
    CULL_MODE_FRONT_AND_BACK_FACE
};

enum ShaderType {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_MAX
};

enum TextureFlag {
    TEXTURE_OPTION_CLAMP_TO_EDGE = 1,
    TEXTURE_OPTION_FLIP_VERTICALLY = 2,
    TEXTURE_OPTION_DISABLE_MIPMAPS = 4,
    TEXTURE_OPTION_NEAREST_FILTER = 8
};

enum VirtualGamepadConfig {
    VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS,
    VIRTUAL_GAMEPAD_CONFIG_HAT_AND_BUTTON
};

const std::string DEFAULT_MATERIAL_SCHEME = "default";

class Mesh;
typedef std::weak_ptr<Mesh> MeshRef;
typedef std::shared_ptr<Mesh> MeshPtr;

class Material;
typedef std::weak_ptr<Material> MaterialRef;
typedef std::shared_ptr<Material> MaterialPtr;

class Texture;
typedef std::weak_ptr<Texture> TextureRef;
typedef std::shared_ptr<Texture> TexturePtr;

class Sound;
typedef std::weak_ptr<Sound> SoundRef;
typedef std::shared_ptr<Sound> SoundPtr;

class Font;
typedef std::weak_ptr<Font> FontRef;
typedef std::shared_ptr<Font> FontPtr;

class Actor;
typedef Actor* ActorPtr;

class Geom;
typedef Geom* GeomPtr;

class ParticleSystem;
typedef ParticleSystem* ParticleSystemPtr;

class Sprite;
typedef Sprite* SpritePtr;

class Light;
typedef Light* LightPtr;

class Camera;
class CameraProxy;

typedef Camera* CameraPtr;
typedef CameraProxy* CameraProxyPtr;

class Viewport;

class Background;
typedef Background* BackgroundPtr;

class Stage;
class WindowBase;
typedef Stage* StagePtr;

namespace ui {

class Widget;
class ProgressBar;
class Button;
class Label;

typedef Widget* WidgetPtr;

}

class ResourceManager;
typedef AutoWeakPtr<ResourceManager> ResourceManagerPtr;

class RenderSequence;
typedef AutoWeakPtr<RenderSequence> RenderSequencePtr;

class Pipeline;
typedef Pipeline* PipelinePtr;

class Frustum;
class WindowBase;
class Partitioner;

class GPUProgram;
typedef std::shared_ptr<GPUProgram> GPUProgramPtr;

class Skybox;
typedef Skybox* SkyboxPtr;

typedef uint32_t IdleConnectionID;

typedef UniqueID<MeshPtr> MeshID;
typedef UniqueID<TexturePtr> TextureID;
typedef UniqueID<FontPtr> FontID;
typedef UniqueID<CameraPtr> CameraID;
typedef UniqueID<MaterialPtr> MaterialID;
typedef UniqueID<LightPtr> LightID;
typedef UniqueID<StagePtr> StageID;
typedef UniqueID<ActorPtr> ActorID;
typedef UniqueID<GeomPtr> GeomID;
typedef UniqueID<SoundPtr> SoundID;
typedef UniqueID<PipelinePtr> PipelineID;
typedef UniqueID<SpritePtr> SpriteID;
typedef UniqueID<BackgroundPtr> BackgroundID;
typedef UniqueID<ParticleSystemPtr> ParticleSystemID;
typedef UniqueID<SkyboxPtr> SkyID;
typedef UniqueID<GPUProgramPtr> GPUProgramID;
typedef UniqueID<ui::WidgetPtr> WidgetID;

typedef generic::TemplatedManager<Stage, StageID> BaseStageManager;

}

/* Hash functions for smlt types */
namespace std {
    template <> struct hash<smlt::MeshArrangement> {
        size_t operator() (smlt::MeshArrangement t) { return size_t(t); }
    };
}



// Generic hash for tuples by Leo Goodstadt
// http://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace std{
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v)
        {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
          static void apply(size_t& seed, Tuple const& tuple)
          {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
          }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0>
        {
          static void apply(size_t& seed, Tuple const& tuple)
          {
            hash_combine(seed, std::get<0>(tuple));
          }
        };
    }

    template <typename ... TT>
    struct hash<std::tuple<TT...>>
    {
        size_t
        operator()(std::tuple<TT...> const& tt) const
        {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }

    };
}


#endif // TYPES_H_INCLUDED
