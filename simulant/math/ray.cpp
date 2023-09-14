#include <limits>
#include "ray.h"

namespace smlt {

bool Ray::intersects_aabb(const AABB &aabb) const {
    //http://gamedev.stackexchange.com/a/18459/15125
    const Vec3 rdir = this->dir.normalized();
    const Vec3 dirfrac(fast_divide(1.0f, rdir.x), fast_divide(1.0f, rdir.y), fast_divide(1.0f, rdir.z));

    const float t1 = (aabb.min().x - start.x) * dirfrac.x;
    const float t2 = (aabb.max().x - start.x) * dirfrac.x;
    const float t3 = (aabb.min().y - start.y) * dirfrac.y;
    const float t4 = (aabb.max().y - start.y) * dirfrac.y;
    const float t5 = (aabb.min().z - start.z) * dirfrac.z;
    const float t6 = (aabb.max().z - start.z) * dirfrac.z;

    const float tmin = fast_max(fast_max(fast_min(t1, t2), fast_min(t3, t4)), fast_min(t5, t6));
    const float tmax = fast_min(fast_min(fast_max(t1, t2), fast_max(t3, t4)), fast_max(t5, t6));

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

    inv_det = fast_divide(1.0f, det);

    T = start - v1;
    u = T.dot(P) * inv_det;

    //The intersection lies outside of the triangle
    if(u < 0.0f || u > 1.0f) {
        return false;
    }

    Q = T.cross(e1);
    v = dir.dot(Q) * inv_det;

    if(v < 0.0f || u + v  > 1.0f) {
        return false;
    }

    t = e2.dot(Q) * inv_det;

    if(t > EPSILON && t <= 1.0f) { //ray intersection
        const float dist = t * dir.length();
        if(distance) {
            *distance = dist;
        }

        if(intersection) {
            *intersection = start + (dir.normalized() * dist);
        }

        if(normal) {
            *normal = e1.cross(e2).normalized(); //make sure the normal is in fact normalized
        }

        return true;
    }

    return false;
}

bool Ray::intersects_sphere(const Vec3& center, const float radius, Vec3 *intersection, Vec3 *normal, float *distance) const {
    //based on https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
    float t0, t1;
    const float r2 = radius * radius;
    const Vec3 L = center - start;

    const float tca = L.dot(dir);
    const float d2 = L.dot(L) - tca * tca;

    if(d2 > r2) return false;

    const float thc = smlt::fast_sqrt(r2 - d2);
    t0 = tca - thc;
    t1 = tca + thc;

    if (t0 > t1) 
        std::swap(t0, t1); 
 
    if (t0 < 0) { 
        t0 = t1; // if t0 is negative, let's use t1 instead 
        if (t0 < 0) return false; // both t0 and t1 are negative 
    } 
 
    const Vec3 pos = start + (dir.normalized() * t0);

    if(distance) {
        *distance = t0;
    }

    if(intersection) {
        *intersection = pos;
    }

    if(normal) {
        *normal = (pos - center).normalized();
    }

    return true; 
}

}
