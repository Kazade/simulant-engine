#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <kazmath/vec3.h>

namespace kglt {

const int FRUSTUM_CORNER_NEAR_BOTTOM_LEFT = 0;
const int FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT = 1;

class Frustum {
public:
    void build_frustum(const kmVec3& camera_position, const kmVec3& camera_forward,
                       double near_distance, double far_distance);

    std::vector<kmVec3> near_corners() const; ///< Returns the near 4 corners of the frustum
    std::vector<kmVec3> far_corners() const; ///< Returns the far 4 corners of the frustum
    bool contains_point(const kmVec3& point) const; ///< Returns true if the frustum contains point

private:
    std::vector<kmVec3> near_corners_;
    std::vector<kmVec3> far_corners_;
};

}

#endif // FRUSTUM_H
