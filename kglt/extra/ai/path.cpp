
#include "path.h"

namespace kglt {
namespace extra {

Path::Path(float radius, bool cyclic):
    OpenSteer::PolylinePathway(0, nullptr, 0.0, false),
    radius_(radius),
    cyclic_(cyclic) {

}

bool Path::empty() const {
    return points_.empty();
}

uint32_t Path::length() const {
    return points_.size();
}

void Path::add_point(const kglt::Vec3 &point) {
    points_.push_back(point);

    initialize(points_.size(), &points_[0], radius_, cyclic_);
}

kglt::Vec3 Path::point(uint32_t idx) const {
    return points_.at(idx);
}

}
}
