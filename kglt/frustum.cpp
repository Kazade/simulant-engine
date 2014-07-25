#include <cassert>
#include "frustum.h"
#include "types.h"

namespace kglt {

Frustum::Frustum():
    initialized_(false) {

}

bool Frustum::intersects_aabb(const kmAABB &aabb) const {
    for(const kmPlane& plane: planes_) {
        kglt::Vec3 points[] = {
            Vec3(aabb.min.x, aabb.min.y, aabb.min.z),
            Vec3(aabb.max.x, aabb.min.y, aabb.min.z),
            Vec3(aabb.max.x, aabb.max.y, aabb.min.z),
            Vec3(aabb.max.x, aabb.max.y, aabb.max.z),
            Vec3(aabb.min.x, aabb.max.y, aabb.min.z),
            Vec3(aabb.min.x, aabb.max.y, aabb.max.z),
            Vec3(aabb.max.x, aabb.min.y, aabb.max.z),
            Vec3(aabb.min.x, aabb.min.y, aabb.max.z)
        };

        int32_t points_behind = 0;
        for(Vec3& p: points) {
            KM_POINT_CLASSIFICATION classify = kmPlaneClassifyPoint(&plane, &p);
            if(classify == POINT_BEHIND_PLANE) {
                points_behind++;
            }
        }

        if(points_behind == 8) {
            return false;
        }
    }

    return true;
}

void Frustum::build(const kmMat4* modelview_projection) {
    planes_.resize(FRUSTUM_PLANE_MAX);

    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_LEFT], modelview_projection, 1);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_RIGHT], modelview_projection, -1);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_BOTTOM], modelview_projection, 2);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_TOP], modelview_projection, -2);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_NEAR], modelview_projection, 3);
    kmPlaneExtractFromMat4(&planes_[FRUSTUM_PLANE_FAR], modelview_projection, 3);

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
    for(const kmPlane& plane: planes_) {
        KM_POINT_CLASSIFICATION classify = kmPlaneClassifyPoint(&plane, &point);
        if(classify == POINT_BEHIND_PLANE) {
            return false;
        }
    }

    return true;
}

}
