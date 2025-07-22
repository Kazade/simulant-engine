#include "bezier_curve.h"

namespace smlt {

float BezierCurve::length(int segment_count) {
    float total_length = 0.0f;
    Vec3 last_point = calc_bezier_point(0.0f);
    for(int i = 1; i <= segment_count; ++i) {
        float t = static_cast<float>(i) / segment_count;
        Vec3 current_point = calc_bezier_point(t);
        total_length += (current_point - last_point).length();
        last_point = current_point;
    }
    return total_length;
}

Vec3 BezierCurve::calc_bezier_point(const NormalizedFloat t) const {

    // Optimized implementation of the deCasteljau algorithm
    const float x = 1.0f - t;
    const float t2 = t * t;
    const float x2 = x * x;
    const float x3 = x2 * x;
    const float t3 = t2 * t;

    const Vec3 bezierPoint_ =  (x3) * bezierPoints_[0] + 
                    (3.0f * x2 * t) * bezierPoints_[1] +
                    (3.0f * x * t2) * bezierPoints_[2] +
                    (t3) * bezierPoints_[3];

    return bezierPoint_;
}
}
