#include "vec4.h"
#include "vec3.h"

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

}
