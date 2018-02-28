//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cassert>
#include "frustum.h"
#include "types.h"

namespace smlt {

Frustum::Frustum():
    initialized_(false) {

}

bool Frustum::intersects_cube(const Vec3& centre, float size) const {
    const float& x = centre.x;
    const float& y = centre.y;
    const float& z = centre.z;

    for(const Plane& plane: planes_) {
        if(plane.n.x * (x - size) + plane.n.y * (y - size) + plane.n.z * (z - size) + plane.d > 0)
          continue;
        if(plane.n.x * (x + size) + plane.n.y * (y - size) + plane.n.z * (z - size) + plane.d > 0)
          continue;
        if(plane.n.x * (x - size) + plane.n.y * (y + size) + plane.n.z * (z - size) + plane.d > 0)
          continue;
        if(plane.n.x * (x + size) + plane.n.y * (y + size) + plane.n.z * (z - size) + plane.d > 0)
          continue;
        if(plane.n.x * (x - size) + plane.n.y * (y - size) + plane.n.z * (z + size) + plane.d > 0)
          continue;
        if(plane.n.x * (x + size) + plane.n.y * (y - size) + plane.n.z * (z + size) + plane.d > 0)
          continue;
        if(plane.n.x * (x - size) + plane.n.y * (y + size) + plane.n.z * (z + size) + plane.d > 0)
          continue;
        if(plane.n.x * (x + size) + plane.n.y * (y + size) + plane.n.z * (z + size) + plane.d > 0)
          continue;
        return false;
    }

    return true;
}

bool Frustum::intersects_aabb(const AABB& aabb) const {
    for(const Plane& plane: planes_) {
        auto points = aabb.corners();

        int32_t points_behind = 0;
        for(Vec3& p: points) {
            auto classification = plane.classify_point(p);

            if(classification == PLANE_CLASSIFICATION_IS_BEHIND_PLANE) {
                points_behind++;
            }
        }

        if(points_behind == 8) {
            return false;
        }
    }

    return true;
}

Vec3 Frustum::direction() const {
    Vec3 far = Vec3::find_average(far_corners());
    Vec3 near = Vec3::find_average(near_corners());
    return (far - near).normalized();
}

float Frustum::aspect_ratio() const {
    return far_width() / far_height();
}

float Frustum::width_at_distance(float distance) const {
    return height_at_distance(distance) * aspect_ratio();
}

float Frustum::height_at_distance(float distance) const {
    const float Deg2Rad = (PI * 2) / 360;
    return 2.0f * distance * tanf(field_of_view().value * 0.5f * Deg2Rad);
}

Degrees Frustum::field_of_view() const {
    float cosb = planes_[FRUSTUM_PLANE_TOP].normal().dot(planes_[FRUSTUM_PLANE_NEAR].normal());
    return Degrees(Radians(2.0f * atanf(cosb * (1.0 / sqrtf(1.0f - cosb * cosb)))));
}

void Frustum::build(const Mat4* modelview_projection) {
    planes_.resize(FRUSTUM_PLANE_MAX);

    planes_[FRUSTUM_PLANE_LEFT] = modelview_projection->extract_plane(FRUSTUM_PLANE_LEFT);
    planes_[FRUSTUM_PLANE_RIGHT] = modelview_projection->extract_plane(FRUSTUM_PLANE_RIGHT);
    planes_[FRUSTUM_PLANE_BOTTOM] = modelview_projection->extract_plane(FRUSTUM_PLANE_BOTTOM);
    planes_[FRUSTUM_PLANE_TOP] = modelview_projection->extract_plane(FRUSTUM_PLANE_TOP);
    planes_[FRUSTUM_PLANE_NEAR] = modelview_projection->extract_plane(FRUSTUM_PLANE_NEAR);
    planes_[FRUSTUM_PLANE_FAR] = modelview_projection->extract_plane(FRUSTUM_PLANE_FAR);

    near_corners_.resize(FRUSTUM_CORNER_MAX);

    near_corners_[FRUSTUM_CORNER_BOTTOM_LEFT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_LEFT],
        planes_[FRUSTUM_PLANE_BOTTOM],
        planes_[FRUSTUM_PLANE_NEAR]
    ).value();

    near_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_RIGHT],
        planes_[FRUSTUM_PLANE_BOTTOM],
        planes_[FRUSTUM_PLANE_NEAR]
    ).value();

    near_corners_[FRUSTUM_CORNER_TOP_RIGHT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_RIGHT],
        planes_[FRUSTUM_PLANE_TOP],
        planes_[FRUSTUM_PLANE_NEAR]
    ).value();

    near_corners_[FRUSTUM_CORNER_TOP_LEFT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_LEFT],
        planes_[FRUSTUM_PLANE_TOP],
        planes_[FRUSTUM_PLANE_NEAR]
    ).value();

    far_corners_.resize(FRUSTUM_CORNER_MAX);
    far_corners_[FRUSTUM_CORNER_BOTTOM_LEFT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_LEFT],
        planes_[FRUSTUM_PLANE_BOTTOM],
        planes_[FRUSTUM_PLANE_FAR]
    ).value();

    far_corners_[FRUSTUM_CORNER_BOTTOM_RIGHT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_RIGHT],
        planes_[FRUSTUM_PLANE_BOTTOM],
        planes_[FRUSTUM_PLANE_FAR]
    ).value();

    far_corners_[FRUSTUM_CORNER_TOP_RIGHT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_RIGHT],
        planes_[FRUSTUM_PLANE_TOP],
        planes_[FRUSTUM_PLANE_FAR]
    ).value();

    far_corners_[FRUSTUM_CORNER_TOP_LEFT] = Plane::intersect_planes(
        planes_[FRUSTUM_PLANE_LEFT],
        planes_[FRUSTUM_PLANE_TOP],
        planes_[FRUSTUM_PLANE_FAR]
    ).value();

    initialized_ = true;
}

std::vector<Vec3> Frustum::near_corners() const {
    return near_corners_;
}

std::vector<Vec3> Frustum::far_corners() const {
    return far_corners_;
}

bool Frustum::contains_point(const Vec3 &point) const {
    for(const Plane& plane: planes_) {
        auto classify = plane.classify_point(point);
        if(classify == PLANE_CLASSIFICATION_IS_BEHIND_PLANE) {
            return false;
        }
    }

    return true;
}

}
