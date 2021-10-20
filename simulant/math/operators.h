#pragma once

#include <cmath>
#include "vec3.h"
#include "quaternion.h"

/* This file is for operators between types, it helps
 * prevent circular dependencies and allows inlining by
 * the compiler */

namespace smlt {

inline Vec3 operator*(const Quaternion& q, const Vec3& v) {
    const Vec3* quat_vector = (Vec3*) (&q.x);
    const Vec3 uv = quat_vector->cross(v);
    const Vec3 uuv = quat_vector->cross(uv);

    Vec3 ret;
    ret.x = fmaf(fmaf(uv.x, q.w, uuv.x), 2.0f, v.x);
    ret.y = fmaf(fmaf(uv.y, q.w, uuv.y), 2.0f, v.y);
    ret.z = fmaf(fmaf(uv.z, q.w, uuv.z), 2.0f, v.z);
    return ret;
}

inline Vec3 operator*(const Vec3& v, const Quaternion& q) {
    return q * v;
}

inline Vec3& operator*=(Vec3& lhs, const Quaternion& rhs) {
    lhs = lhs * rhs;
    return lhs;
}

}
