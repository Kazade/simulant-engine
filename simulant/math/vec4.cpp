#include "vec4.h"
#include "vec3.h"
#include "vec2.h"

namespace smlt {

std::ostream& operator<<(std::ostream& stream, const Vec4& vec) {
    stream << "(" << vec.x << "," << vec.y << "," << vec.z << "," << vec.w << ")";
    return stream;
}

Vec4::Vec4(const Vec3 &v, float w) {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    this->w = w;
}

Vec3 Vec4::xyz() const {
    return Vec3(x, y, z);
}

Vec2 Vec4::xy() const {
    return Vec2(x, y);
}

}
