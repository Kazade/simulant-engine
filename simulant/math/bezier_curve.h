#pragma once

#include <cmath>
#include <array>
#include "vec3.h"
#include "quaternion.h"

namespace smlt {

struct BezierCurve {

// Creates a degree-three bezier curve (cubic)
BezierCurve(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3):
    p0_(p0),
    p1_(p1),
    p2_(p2),
    p3_(p3) {
        bezierPoints_.clear();
        bezierPoints_.push_back(p0_);
        bezierPoints_.push_back(p1_);
        bezierPoints_.push_back(p2_);
        bezierPoints_.push_back(p3_);
    }

// Returns the position and rotation of a point on the curve at a given time.
// Please note that 't' will be clamped between 0 and 1.
Vec3 calc_bezier_point(const RangeValue<0, 1> t) const;

private:
std::vector<Vec3> bezierPoints_;

Vec3 p0_, p1_, p2_, p3_;

};
}