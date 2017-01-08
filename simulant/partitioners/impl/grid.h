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

#pragma once

#include "../../types.h"

namespace smlt {
namespace octree_impl {


class Grid {
public:
    Grid(float step, float range, float offset_x, float offset_y, float offset_z):
        step_(step),
        range_(range),
        offset_x_(offset_x),
        offset_y_(offset_y),
        offset_z_(offset_z) {

    }

    float snap_x(float x) {
        auto gridx = std::round((x - offset_x_) / step_);
        return std::max(-range_, std::min(range_, gridx * step_ + offset_x_));
    }

    float snap_y(float y) {
        auto gridy = std::round((y - offset_y_) / step_);
        return std::max(-range_, std::min(range_, gridy * step_ + offset_y_));
    }

    float snap_z(float z) {
        auto gridz = std::round((z - offset_z_) / step_);
        return std::max(-range_, std::min(range_, gridz * step_ + offset_z_));
    }

    Vec3 snap(const Vec3& p) {
        return Vec3(
            snap_x(p.x),
            snap_y(p.y),
            snap_z(p.z)
        );
    }

private:
    float step_;
    float range_;
    float offset_x_;
    float offset_y_;
    float offset_z_;
};


}
}
