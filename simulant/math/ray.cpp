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

static bool intersect_line_triangle(const Vec3& orig, const Vec3& dir,
    const Vec3& vert0, const Vec3& vert1, const Vec3& vert2,
    Vec3& position
) {
    const float epsilon = std::numeric_limits<float>::epsilon();

    auto edge1 = vert1 - vert0;
    auto edge2 = vert2 - vert0;
    auto pvec = dir.cross(edge2);

    float det = edge1.dot(pvec);

    if(det > -epsilon && det < epsilon) {
        return false;
    }

    float inv_det = 1.0f / det;

    auto tvec = orig - vert0;

    position.y = tvec.dot(pvec) * inv_det;

    if(position.y < 0.0f || position.y > 1.0f) {
        return false;
    }

    auto qvec = tvec.cross(edge1);

    position.z = dir.dot(qvec) * inv_det;

    if(position.z < 0.0f || position.y + position.z > 1.0f) {
        return false;
    }

    position.x = edge2.dot(qvec) * inv_det;

    return true;
}

bool Ray::intersects_triangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3, Vec3 *intersection, Vec3 *normal, float *distance) const {
    Vec3 hit;
    bool ret = intersect_line_triangle(start, dir, v1, v2, v3, hit);

    if(ret) {
        if(intersection) *intersection = hit;
        if(normal) {
            *normal = (v2 - v1).cross(v3 - v1).normalized();
        }

        if(distance) {
            *distance = (hit - start).length();
        }
    }

    return ret;
}


}
