#include <cassert>
#include "frustum.h"

namespace frustum {

void Frustum::build_frustum(const kmVec3& camera_position, const kmVec3& camera_forward,
                   double near_distance, double far_distance) {
    assert(0 && "Not implemented");
}

std::vector<kmVec3> Frustum::near_corners() const {
    assert(0 && "Not implemented");
}

std::vector<kmVec3> Frustum::far_corners() const {
    assert(0 && "Not implemented");
}

bool Frustum::contains_point(const kmVec3& point) const {
    assert(0 && "Not implemented");
}

}
