#include "../types.h"
#include "../utils/random.h"

#include "vec3.h"
#include "aabb.h"

namespace smlt {

std::ostream& operator<<(std::ostream& stream, const Vec3& vec) {
    stream << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return stream;
}

Vec3::Vec3(const Vec2 &v2, float z):
    x(v2.x), y(v2.y), z(z) {

}

Vec3 Vec3::rotated_by(const Quaternion &q) const {
    return q * (*this);
}

Vec3 Vec3::rotated_by(const Mat3 &rot) const {
    return rot.transform_vector(*this);
}

Vec3 operator*(float lhs, const Vec3& rhs) {
    smlt::Vec3 result = rhs;
    result *= lhs;
    return result;
}

Vec3 operator/(float lhs, const Vec3& rhs) {
    smlt::Vec3 result = rhs;
    float l = fast_divide(1.0f, lhs);
    return result * l;
}

Vec3 Vec3::random_deviant(const Degrees& angle) const {
    auto v = normalized();
    auto& rgen = RandomGenerator::instance();

    smlt::Radians theta(
        rgen.float_in_range(-angle.to_float() * smlt::PI_OVER_180,
                            angle.to_float() * smlt::PI_OVER_180));

    auto r = smlt::Vec3(1.0f, 0.0f, 0.0f);
    if(std::abs(v.x) > 0.9f) {
        r = smlt::Vec3(0.0f, 1.0f, 0.0f);
    }

    auto w = v.cross(r).normalized();

    float cosTheta = std::cos(theta.to_float());
    float sinTheta = std::sin(theta.to_float());

    return (v * cosTheta) + (w * sinTheta) +
           (w * (v.dot(w) * (1.0f - cosTheta)));
}

Vec2 Vec3::xy() const {
    return Vec2(x, y);
}

Vec4 Vec3::xyzw(float w) const {
    return Vec4(x, y, z, w);
}

Quaternion Vec3::rotation_to(const Vec3 &dir) const {
    float d = dot(dir);
    if(d >= 1.0f) {
       return Quaternion();
    } else if(d < (EPSILON - 1.0f)) {
        Vec3 axis = Vec3::right().cross(*this);
        if(fast_abs(axis.length_squared()) < EPSILON) {
            axis = Vec3::up().cross(*this);
       }
       axis.normalize();
       return smlt::Quaternion(axis, smlt::Radians(smlt::PI));
    } else {
        float s = fast_sqrt((1.0f + d) * 2.0f);
        float inv = fast_divide(1.0f, s);
        Vec3 c = cross(dir);
        return Quaternion(
            c.x * inv,
            c.y * inv,
            c.z * inv,
            s * 0.5f
        ).normalized();
   }
}

Vec3 Vec3::rotated_by(const Mat4 &rot) const {
    // Avoid going through the operator for performance
    const float* m = &rot[0];

    return Vec3(
        x * m[0] + y * m[4] + z * m[8],
        x * m[1] + y * m[5] + z * m[9],
        x * m[2] + y * m[6] + z * m[10]
    );
}

Vec3 Vec3::transformed_by(const Mat4 &trans) const {
    // Avoid going through the operator for performance
    const float* m = &trans[0];

    return Vec3(
        x * m[0] + y * m[4] + z * m[8] + 1.0f * m[12],
        x * m[1] + y * m[5] + z * m[9] + 1.0f * m[13],
        x * m[2] + y * m[6] + z * m[10] + 1.0f * m[14]
    );
}

Vec3 Vec3::perpendicular() const {
    return (z < x) ? Vec3(y, -x, 0) : Vec3(0, -z, y);
}

smlt::Vec3 operator-(const smlt::Vec3& vec) {
    return smlt::Vec3(-vec.x, -vec.y, -vec.z);
}

float Vec3::distance_to(const AABB& aabb) const {
    Vec3 center = aabb.center();

    float dx = fast_max(fast_abs(x - center.x) - (aabb.width() * 0.5f), 0.0f);
    float dy = fast_max(fast_abs(y - center.y) - (aabb.height() * 0.5f), 0.0f);
    float dz = fast_max(fast_abs(z - center.z) - (aabb.depth() * 0.5f), 0.0f);

    Vec3 d = Vec3(dx, dy, dz);
    return fast_sqrt(d.dot(d));
}

}
