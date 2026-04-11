#pragma once

#include "../math/vec2.h"
#include "../math/vec3.h"

namespace smlt {

struct Quaternion;
struct Radians;

/** Linear interpolation from x to y with factor t, where t can be any value
 * between 0 and 1 */
inline float lerp(const float x, const float y, const float t) {
    return ::fmaf((y - x), t, x);
}

inline Vec2 lerp(const Vec2& x, const Vec2& y, const float t) {
    return x + ((y - x) * t);
}

inline Vec3 lerp(const Vec3& x, const Vec3& y, const float t) {
    return x + ((y - x) * t);
}

Quaternion lerp(const Quaternion& x, const Quaternion& y, const float t);
Radians lerp(const Radians& a, const Radians& b, float t);
Degrees lerp(Degrees a, Degrees b, float t);

/** Frame rate independent interpolation from x to y:
 * dt is the delta time in seconds
 * p is the target precision (e.g. 0.01f which would be 1% of distance
 * remaining) t is the expected positive duration until the remaining distance
 * matches the target precision p */
template<typename T>
inline T lerp_smooth(const T x, const T y, const float dt, const float p,
                     const float t) {
    return lerp(x, y, 1.0f - std::pow(p, fast_divide(dt, t)));
}

} // namespace smlt
