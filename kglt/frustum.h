#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <kazmath/vec3.h>
#include <kazmath/plane.h>

#include <vector>

namespace kglt {

enum FrustumCorner {
    FRUSTUM_CORNER_BOTTOM_LEFT = 0,
    FRUSTUM_CORNER_BOTTOM_RIGHT,
    FRUSTUM_CORNER_TOP_RIGHT,
    FRUSTUM_CORNER_TOP_LEFT,
    FRUSTUM_CORNER_MAX
};

enum FrustumPlane {
    FRUSTUM_PLANE_LEFT = 0,
    FRUSTUM_PLANE_RIGHT,
    FRUSTUM_PLANE_BOTTOM,
    FRUSTUM_PLANE_TOP,
    FRUSTUM_PLANE_NEAR,
    FRUSTUM_PLANE_FAR,
    FRUSTUM_PLANE_MAX
};

class Frustum {
public:
    Frustum();

    void build_frustum(const kmMat4* modelview_projection);

    std::vector<kmVec3> near_corners() const; ///< Returns the near 4 corners of the frustum
    std::vector<kmVec3> far_corners() const; ///< Returns the far 4 corners of the frustum
    bool contains_point(const kmVec3& point) const; ///< Returns true if the frustum contains point
    bool initialized() const { return initialized_; }

private:
    bool initialized_;

    std::vector<kmVec3> near_corners_;
    std::vector<kmVec3> far_corners_;
    std::vector<kmPlane> planes_;
};

}

#endif // FRUSTUM_H
