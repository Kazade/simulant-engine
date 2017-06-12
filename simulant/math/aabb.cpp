#include <limits>
#include "aabb.h"

namespace smlt {

AABB::AABB(const Vec3 &min, const Vec3 &max) {
    set_min(min);
    set_max(max);
    rebuild_corners();
}

AABB::AABB(const Vec3 &centre, float width) {
    set_min(centre - Vec3(width * 0.5, width * 0.5, width * 0.5));
    set_max(centre + Vec3(width * 0.5, width * 0.5, width * 0.5));

    rebuild_corners();
}

AABB::AABB(const Vec3 &centre, float xsize, float ysize, float zsize) {
    set_min(centre - Vec3(xsize * 0.5, ysize * 0.5, zsize * 0.5));
    set_max(centre + Vec3(xsize * 0.5, ysize * 0.5, zsize * 0.5));

    rebuild_corners();
}

AABB::AABB(const Vec3 *vertices, const std::size_t count) {
    if(count == 0) {
        set_min(Vec3());
        set_max(Vec3());
        rebuild_corners();
        return;
    }

    float minx = std::numeric_limits<float>::max();
    float miny = std::numeric_limits<float>::max();
    float minz = std::numeric_limits<float>::max();

    float maxx = std::numeric_limits<float>::lowest();
    float maxy = std::numeric_limits<float>::lowest();
    float maxz = std::numeric_limits<float>::lowest();

    for(uint32_t i = 0; i < count; ++i) {
        if(vertices[i].x < minx) minx = vertices[i].x;
        if(vertices[i].y < miny) miny = vertices[i].y;
        if(vertices[i].z < minz) minz = vertices[i].z;

        if(vertices[i].x > maxx) maxx = vertices[i].x;
        if(vertices[i].y > maxy) maxy = vertices[i].y;
        if(vertices[i].z > maxz) maxz = vertices[i].z;
    }

    set_min(Vec3(minx, miny, minz));
    set_max(Vec3(maxx, maxy, maxz));

    rebuild_corners();
}

}
