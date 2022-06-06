
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

#include "../types.h"

namespace smlt {

const Vec2 Vec2::NEGATIVE_X = Vec2(-1, 0);
const Vec2 Vec2::POSITIVE_X = Vec2(1, 0);
const Vec2 Vec2::NEGATIVE_Y = Vec2(0, -1);
const Vec2 Vec2::POSITIVE_Y = Vec2(0, 1);

const Vec2 Vec2::DOWN = Vec2::NEGATIVE_Y;       //Shorthand for writing Vec2(0, -1)
const Vec2 Vec2::LEFT = Vec2::NEGATIVE_X;       //Shorthand for writing Vec2(-1, 0)
const Vec2 Vec2::ONE = Vec2(1, 1);              //Shorthand for writing Vec2(1, 1)
const Vec2 Vec2::RIGHT = Vec2::POSITIVE_X;      //Shorthand for writing Vec2(1, 0)
const Vec2 Vec2::UP = Vec2::POSITIVE_Y;         //Shorthand for writing Vec2(0, 1)
const Vec2 Vec2::ZERO = Vec2();                 //Shorthand for writing Vec2(0, 0)

Vec2 Vec2::rotated_by(const Degrees& degrees) const {
    float r = Radians(degrees).value;
    float cosR = cos(r);
    float sinR = sin(r);

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
