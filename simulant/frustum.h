/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

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

enum FrustumClassification {
    FRUSTUM_CONTAINS_NONE = 0,
    FRUSTUM_CONTAINS_PARTIAL,
    FRUSTUM_CONTAINS_ALL
};

class Frustum {
public:
    Frustum();

    void build(const Mat4* modelview_projection);

    std::vector<Vec3> near_corners() const; ///< Returns the near 4 corners of the frustum
    std::vector<Vec3> far_corners() const; ///< Returns the far 4 corners of the frustum

    bool contains_point(const Vec3& point) const; ///< Returns true if the frustum contains point
    bool intersects_aabb(const AABB &box) const;
    bool intersects_cube(const Vec3& centre, float size) const;

    bool initialized() const { return initialized_; }

    double near_height() const {
        assert(initialized_);
        return (near_corners_[FRUSTUM_CORNER_BOTTOM_LEFT] - near_corners_[FRUSTUM_CORNER_TOP_LEFT]).length();
    }

    double far_height() const {
        assert(initialized_);
        return (far_corners_[FRUSTUM_CORNER_BOTTOM_LEFT] - far_corners_[FRUSTUM_CORNER_TOP_LEFT]).length();
    }

    double near_width() const {
        assert(initialized_);
        return (near_corners_[FRUSTUM_CORNER_BOTTOM_LEFT] - near_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT]).length();
    }

    double far_width() const {
        assert(initialized_);
        return (far_corners_[FRUSTUM_CORNER_BOTTOM_LEFT] - far_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT]).length();
    }


    double depth() const {
        assert(initialized_);

        /*
         * We need to find the central points of both the near and far corners
         * and return the length between them
         */
        Vec3 near_avg, far_avg;

        for(uint32_t i = 0; i < FRUSTUM_CORNER_MAX; ++i) {
            near_avg += near_corners_[i];
            far_avg += far_corners_[i];
        }

        near_avg /= FRUSTUM_CORNER_MAX;
        far_avg /= FRUSTUM_CORNER_MAX;

        return (far_avg - near_avg).length();
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
