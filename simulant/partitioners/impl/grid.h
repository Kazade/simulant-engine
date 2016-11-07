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
