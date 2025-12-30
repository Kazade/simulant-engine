
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

#include "../types.h"

namespace smlt {

Vec2 Vec2::rotated_by(const Degrees& degrees) const {
    float r = Radians(degrees).to_float();
    float cosR = 0.0f;
    float sinR = 0.0f;
    fast_sincos(r, &sinR, &cosR);

    return Vec2(
        x * cosR - y * sinR,
        x * sinR - y * cosR
    );
}

Vec3 Vec2::xyz(float z) const {
    return Vec3(x, y, z);
}

Vec4 Vec2::xyzw(float z, float w) const {
    return Vec4(x, y, z, w);
}


bool operator==(const Vec2& lhs, const Vec2& rhs) {
    return (lhs - rhs).length_squared() < EPSILON;
}

bool operator!=(const Vec2& lhs, const Vec2& rhs) {
    return !(lhs == rhs);
}

smlt::Vec2 operator*(float lhs, const smlt::Vec2& rhs) {
    return Vec2(
        rhs.x * lhs,
        rhs.y * lhs
    );
}

std::ostream& operator<<(std::ostream& stream, const Vec2& vec) {
    stream << "(" << vec.x << "," << vec.y << ")";
    return stream;
}

}
