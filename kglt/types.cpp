
#include "types.h"

namespace kglt {

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

std::ostream& operator<<(std::ostream& stream, const Vec3& vec) {
    stream << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Quaternion& quat) {
    stream << "(" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << ")";
    return stream;
}


}
