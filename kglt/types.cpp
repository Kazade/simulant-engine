
#include "types.h"

namespace kglt {

Radians to_radians(const Degrees& degrees) {
    return Radians(kmDegreesToRadians(degrees.value_));
}

Degrees to_degrees(const Radians& radians) {
    return Degrees(kmRadiansToDegrees(radians.value_));
}

kglt::Vec2 operator*(float lhs, const kglt::Vec2& rhs) {
    kglt::Vec2 result;
    kmVec2Scale(&result, &rhs, lhs);
    return result;
}

kglt::Vec3 operator*(float lhs, const kglt::Vec3& rhs) {
    kglt::Vec3 result;
    kmVec3Scale(&result, &rhs, lhs);
    return result;
}

kglt::Vec3 operator/(float lhs, const kglt::Vec3& rhs) {
    kglt::Vec3 result;
    kmVec3Scale(&result, &rhs, 1.0 / lhs);
    return result;
}

kglt::Vec3 operator-(const kglt::Vec3& vec) {
    return kglt::Vec3(-vec.x, -vec.y, -vec.z);
}

kglt::Quaternion operator-(const kglt::Quaternion& q) {
    return kglt::Quaternion(-q.x, -q.y, -q.z, -q.w);
}

std::ostream& operator<<(std::ostream& stream, const Vec2& vec) {
    stream << "(" << vec.x << "," << vec.y << ")";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Vec3& vec) {
    stream << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Quaternion& quat) {
    stream << "(" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << ")";
    return stream;
}

Quaternion Quaternion::look_rotation(const Vec3& direction, const Vec3& up=Vec3(0, 1, 0)) {
    Quaternion res;
    kmQuaternionLookRotation(&res, &direction, &up);
    return res;
}

}
