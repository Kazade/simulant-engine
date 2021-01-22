#pragma once

#include "vec3.h"
#include "aabb.h"

namespace smlt {

struct Ray {
    Vec3 start;
    Vec3 dir;
    Vec3 dir_inv;

    Ray() = default;

    Ray(const Vec3& start, const Vec3& dir):
        start(start),
        dir(dir) {

        dir_inv = Vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    }

    bool intersects_aabb(const AABB& aabb) const;

    bool intersects_triangle(
        const Vec3& v1, const Vec3& v2, const Vec3& v3,
        Vec3* intersection=nullptr, Vec3* normal=nullptr, float* distance=nullptr
    ) const;

    bool intersects_sphere(const Vec3& center, const float radius, Vec3* intersection=nullptr, Vec3* normal = nullptr, float* distance=nullptr) const;
};


}
