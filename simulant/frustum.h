#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <cstdint>
#include <vector>

#include "types.h"

namespace smlt {

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

enum FrustumClassification {
    FRUSTUM_CONTAINS_NONE = 0,
    FRUSTUM_CONTAINS_PARTIAL,
    FRUSTUM_CONTAINS_ALL
};

class Frustum {
public:
    Frustum();

    void build(const kmMat4* modelview_projection);

    std::vector<Vec3> near_corners() const; ///< Returns the near 4 corners of the frustum
    std::vector<Vec3> far_corners() const; ///< Returns the far 4 corners of the frustum

    bool contains_point(const kmVec3& point) const; ///< Returns true if the frustum contains point
    bool intersects_aabb(const kmAABB3& box) const;

    bool initialized() const { return initialized_; }

    double near_height() const {
        assert(initialized_);
        kmVec3 diff;
        kmVec3Subtract(&diff, &near_corners_[FRUSTUM_CORNER_BOTTOM_LEFT], &near_corners_[FRUSTUM_CORNER_TOP_LEFT]);
        return kmVec3Length(&diff);
    }

    double far_height() const {
        assert(initialized_);
        kmVec3 diff;
        kmVec3Subtract(&diff, &far_corners_[FRUSTUM_CORNER_BOTTOM_LEFT], &far_corners_[FRUSTUM_CORNER_TOP_LEFT]);
        return kmVec3Length(&diff);
    }

    double near_width() const {
        assert(initialized_);
        kmVec3 diff;
        kmVec3Subtract(&diff, &near_corners_[FRUSTUM_CORNER_BOTTOM_LEFT], &near_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT]);
        return kmVec3Length(&diff);
    }

    double far_width() const {
        assert(initialized_);
        kmVec3 diff;
        kmVec3Subtract(&diff, &far_corners_[FRUSTUM_CORNER_BOTTOM_LEFT], &far_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT]);
        return kmVec3Length(&diff);
    }


    double depth() const {
        assert(initialized_);

        /*
         * We need to find the central points of both the near and far corners
         * and return the length between them
         */
        kmVec3 near_avg, far_avg;
        kmVec3Zero(&near_avg);
        kmVec3Zero(&far_avg);
        for(uint32_t i = 0; i < FRUSTUM_CORNER_MAX; ++i) {
            kmVec3Add(&near_avg, &near_avg, &near_corners_[i]);
            kmVec3Add(&far_avg, &far_avg, &far_corners_[i]);
        }

        kmVec3Scale(&near_avg, &near_avg, 1.0 / FRUSTUM_CORNER_MAX);
        kmVec3Scale(&far_avg, &far_avg, 1.0 / FRUSTUM_CORNER_MAX);

        kmVec3 diff;
        kmVec3Subtract(&diff, &far_avg, &near_avg);
        return kmVec3Length(&diff);

    }    

    Plane plane(FrustumPlane p) const {
        return planes_[p];
    }

    Vec3 direction() const;

    float width_at_distance(float distance) const;
    float height_at_distance(float distance) const;
    Degrees field_of_view() const;
    float aspect_ratio() const;
private:
    bool initialized_;

    std::vector<Vec3> near_corners_;
    std::vector<Vec3> far_corners_;
    std::vector<Plane> planes_;
};

}

#endif // FRUSTUM_H
