#include "bezier_curve.h"

namespace smlt {

Vec3 BezierCurve::get_bezier_point(const float t) const {

    Vec3 bezierPoint_;
    Quaternion bezierRotation_;

    if(initialized_ == false)
        return Vec3();

    // Optimized implementation of the deCasteljau algorithm
    const float x = 1.0f - t;
    const float t2 = t * t;
    const float x2 = x * x;
    const float x3 = x2 * x;
    const float t3 = t2 * t;

    bezierPoint_ =  (x3) * bezierPoints_[0] + 
                    (3.0f * x2 * t) * bezierPoints_[1] +
                    (3.0f * x * t2) * bezierPoints_[2] +
                    (t3) * bezierPoints_[3];

    return bezierPoint_;

}

}