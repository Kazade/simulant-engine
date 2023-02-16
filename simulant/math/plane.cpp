#include <limits>

#include "plane.h"
#include "utils.h"

namespace smlt {

Vec3 Plane::project(const Vec3 &p) {
    return p - (n.dot(p) + d) * n;
}

float Plane::distance_to(const Vec3 &p) const {
    float k1 = d;
    float k2 = n.dot(p);
    return k2 - k1;
}

PlaneClassification Plane::classify_point(const Vec3 &p) const {
    const float E = std::numeric_limits<float>::epsilon();

    /* This function will determine if a point is on, in front of, or behind*/
    /* the plane.  First we store the dot product of the plane and the point.*/
    auto distance = n.dot(p) + d;

    /* Simply put if the dot product is greater than 0 then it is infront of it.*/
    /* If it is less than 0 then it is behind it.  And if it is 0 then it is on it.*/
    if(distance > E) return PLANE_CLASSIFICATION_IS_IN_FRONT_OF_PLANE;
    else if(distance < -E) return PLANE_CLASSIFICATION_IS_BEHIND_PLANE;
    else return PLANE_CLASSIFICATION_IS_ON_PLANE;
}

smlt::optional<Vec3> Plane::intersect_planes(const Plane &p1, const Plane &p2, const Plane &p3) {
    const Vec3& n1 = p1.n;
    const Vec3& n2 = p2.n;
    const Vec3& n3 = p3.n;

    auto cross = n2.cross(n3);
    auto denom = n1.dot(cross);

    if(almost_equal(denom, 0.0f)) {
        return smlt::optional<Vec3>();
    }

    auto r1 = n2.cross(n3);
    auto r2 = n3.cross(n1);
    auto r3 = n1.cross(n2);

    r1 *= -p1.d;
    r2 *= p2.d;
    r3 *= p3.d;

    auto ret = r1 - r2 - r3;

    ret *= 1.0f / denom;

    return smlt::optional<Vec3>(std::move(ret));
}

}
