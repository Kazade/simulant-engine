#include <limits>
#include "ray.h"

namespace smlt {

bool Ray::intersects_aabb(const AABB &aabb) const {
    //http://gamedev.stackexchange.com/a/18459/15125
    Vec3 rdir = this->dir.normalized();
    Vec3 dirfrac(1.0f / rdir.x, 1.0f / rdir.y, 1.0f / rdir.z);

    float t1 = (aabb.min().x - start.x) * dirfrac.x;
    float t2 = (aabb.max().x - start.x) * dirfrac.x;
    float t3 = (aabb.min().y - start.y) * dirfrac.y;
    float t4 = (aabb.max().y - start.y) * dirfrac.y;
    float t5 = (aabb.min().z - start.z) * dirfrac.z;
    float t6 = (aabb.max().z - start.z) * dirfrac.z;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
    if(tmax < 0) {
        return false;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return false;
    }

    return false;
}

bool Ray::intersects_triangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3, Vec3 *intersection, Vec3 *normal, float *distance) const {
    Vec3 e1, e2;  //Edge1, Edge2
    Vec3 P, Q, T;
    float det, inv_det, u, v;
    float t;

    e1 = v2 - v1;
    e2 = v3 - v1;

    P = dir.cross(e2);
    det = e1.dot(P);

    if(det > -EPSILON && det < EPSILON) {
        return false;
    }

    inv_det = 1.f / det;

    T = start - v1;
    u = T.dot(P) * inv_det;

    //The intersection lies outside of the triangle
    if(u < 0.f || u > 1.f) {
        return false;
    }

    Q = T.cross(e1);
    v = dir.dot(Q) * inv_det;

    if(v < 0.f || u + v  > 1.f) {
        return false;
    }

    t = e2.dot(Q) * inv_det;

    if(t > EPSILON && t <= 1.0f) { //ray intersection
        float dist = t * dir.length();
        if(distance) {
            *distance = dist;
        }

        if(intersection) {
            *intersection = start + (dir.normalized() * dist);
        }

        if(normal) {
            *normal = e1.cross(e2);
        }

        return true;
    }

    return false;
}


}
