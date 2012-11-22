#include <cassert>
#include "frustum.h"

namespace kglt {

Frustum::Frustum():
    initialized_(false) {

}

FrustumClassification Frustum::contains_aabb(const kmAABB& box) const {
    /**
     * This should return FRUSTUM_CONTAINS_PARTIAL if the box full encompasses
     * the frustum, or if any of the boxes corners are contained within the frustum
     */

    assert(0);
}

void Frustum::build(const kmMat4* modelview_projection) {
    planes_.resize(FRUSTUM_PLANE_MAX);

    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_LEFT], modelview_projection, 1);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_RIGHT], modelview_projection, -1);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_BOTTOM], modelview_projection, 2);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_TOP], modelview_projection, -2);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_NEAR], modelview_projection, 3);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_FAR], modelview_projection, -3);

    near_corners_.resize(FRUSTUM_CORNER_MAX);
    kmPlaneGetIntersection(
        &near_corners_[FRUSTUM_CORNER_BOTTOM_LEFT],
        &planes_[FRUSTUM_PLANE_LEFT],
        &planes_[FRUSTUM_PLANE_BOTTOM],
        &planes_[FRUSTUM_PLANE_NEAR]
    );

    kmPlaneGetIntersection(
        &near_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT],
        &planes_[FRUSTUM_PLANE_RIGHT],
        &planes_[FRUSTUM_PLANE_BOTTOM],
        &planes_[FRUSTUM_PLANE_NEAR]
    );

    kmPlaneGetIntersection(
        &near_corners_[FRUSTUM_CORNER_TOP_RIGHT],
        &planes_[FRUSTUM_PLANE_RIGHT],
        &planes_[FRUSTUM_PLANE_TOP],
        &planes_[FRUSTUM_PLANE_NEAR]
    );

    kmPlaneGetIntersection(
        &near_corners_[FRUSTUM_CORNER_TOP_LEFT],
        &planes_[FRUSTUM_PLANE_LEFT],
        &planes_[FRUSTUM_PLANE_TOP],
        &planes_[FRUSTUM_PLANE_NEAR]
    );

    far_corners_.resize(FRUSTUM_CORNER_MAX);
    kmPlaneGetIntersection(
        &far_corners_[FRUSTUM_CORNER_BOTTOM_LEFT],
        &planes_[FRUSTUM_PLANE_LEFT],
        &planes_[FRUSTUM_PLANE_BOTTOM],
        &planes_[FRUSTUM_PLANE_FAR]
    );

    kmPlaneGetIntersection(
        &far_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT],
        &planes_[FRUSTUM_PLANE_RIGHT],
        &planes_[FRUSTUM_PLANE_BOTTOM],
        &planes_[FRUSTUM_PLANE_FAR]
    );

    kmPlaneGetIntersection(
        &far_corners_[FRUSTUM_CORNER_TOP_RIGHT],
        &planes_[FRUSTUM_PLANE_RIGHT],
        &planes_[FRUSTUM_PLANE_TOP],
        &planes_[FRUSTUM_PLANE_FAR]
    );

    kmPlaneGetIntersection(
        &far_corners_[FRUSTUM_CORNER_TOP_LEFT],
        &planes_[FRUSTUM_PLANE_LEFT],
        &planes_[FRUSTUM_PLANE_TOP],
        &planes_[FRUSTUM_PLANE_FAR]
    );


    initialized_ = true;
}

std::vector<kmVec3> Frustum::near_corners() const {
    return near_corners_;
}

std::vector<kmVec3> Frustum::far_corners() const {
    return far_corners_;
}

bool Frustum::contains_point(const kmVec3& point) const {
    assert(0 && "Not implemented");
}

}
