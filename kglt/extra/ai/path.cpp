
#include "path.h"

namespace kglt {
namespace extra {

Path::Path() {

}

bool Path::empty() const {
    return points_.empty();
}

uint32_t Path::length() const {
    return points_.size();
}

void Path::add_point(const kglt::Vec3 &point) {
    points_.push_back(point);
}

kglt::Vec3 Path::point(uint32_t idx) const {
    return points_.at(idx);
}

}
}
