#pragma once

#include "../generic/range_value.h"
#include "quaternion.h"
#include "vec3.h"
#include <array>
#include <cmath>

namespace smlt {

class BezierCurve {
public:
    // Creates a degree-three bezier curve (cubic)
    BezierCurve(const Vec3& p0, const Vec3& p1, const Vec3& p2,
                const Vec3& p3) {
        bezierPoints_.push_back(p0);
        bezierPoints_.push_back(p1);
        bezierPoints_.push_back(p2);
        bezierPoints_.push_back(p3);
    }

    // Returns the position and rotation of a point on the curve at a given
    // time. Please note that 't' will be clamped between 0 and 1.
    Vec3 calc_bezier_point(const RangeValue<0, 1> t) const;

    float length(int segment_count = 10);

private:
    std::vector<Vec3> bezierPoints_;
};
} // namespace smlt
