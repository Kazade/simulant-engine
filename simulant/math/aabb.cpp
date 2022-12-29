#include <limits>
#include "aabb.h"

#include "../vertex_data.h"

namespace smlt {

AABB::AABB(const Vec3 &centre, float width) {
    center_ = centre;
    extents_ = Vec3(width * 0.5f, width * 0.5f, width * 0.5f);
}

AABB::AABB(const Vec3 &centre, float xsize, float ysize, float zsize) {
    center_ = centre;
    extents_ = Vec3(xsize * 0.5f, ysize * 0.5f, zsize * 0.5f);
}

AABB::AABB(const VertexData& vertex_data) {
    if(vertex_data.empty()) {
        extents_ = Vec3();
        center_ = Vec3();
        return;
    }

    float minx = std::numeric_limits<float>::max();
    float miny = std::numeric_limits<float>::max();
    float minz = std::numeric_limits<float>::max();

    float maxx = std::numeric_limits<float>::lowest();
    float maxy = std::numeric_limits<float>::lowest();
    float maxz = std::numeric_limits<float>::lowest();

    const uint8_t* p = vertex_data.data() + vertex_data.vertex_specification().position_offset();
    bool twod = vertex_data.vertex_specification().position_attribute == VERTEX_ATTRIBUTE_2F;

    for(std::size_t i = 0; i < vertex_data.count(); ++i) {
        float* x = (float*) p;
        float* y = (float*) (p + sizeof(float));

        if(*x < minx) minx = *x;
        if(*y < miny) miny = *y;
        if(*x > maxx) maxx = *x;
        if(*y > maxy) maxy = *y;

        if(!twod) {
            float* z = (float*) (p + sizeof(float) + sizeof(float));
            if(*z < minz) minz = *z;
            if(*z > maxz) maxz = *z;
        } else {
            minz = maxz = 0.0f;
        }

        p += vertex_data.vertex_specification().stride();
    }

    set_min_max(Vec3(minx, miny, minz), Vec3(maxx, maxy, maxz));
}

AABB::AABB(const Vec3 *vertices, const std::size_t count) {
    if(count == 0) {
        extents_ = Vec3();
        center_ = Vec3();
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

    set_min_max(Vec3(minx, miny, minz), Vec3(maxx, maxy, maxz));
}

bool AABB::intersects_aabb(const AABB &bounds) const {
    auto lmin = min();
    auto rmin = bounds.min();
    auto lmax = max();
    auto rmax = bounds.max();

    return (lmin.x <= rmax.x) && (lmax.x >= rmin.x) &&
        (lmin.y <= rmax.y) && (lmax.y >= rmin.y) &&
        (lmin.z <= rmax.z) && (lmax.z >= rmin.z);

    return true;
}

bool AABB::intersects_sphere(const smlt::Vec3& center, float diameter) const {
    float radius = diameter * 0.5f;

    auto ex = std::max(min().x - center.x, 0.0f) + std::max(center.x - max().x, 0.0f);
    auto ey = std::max(min().y - center.y, 0.0f) + std::max(center.y - max().y, 0.0f);
    auto ez = std::max(min().z - center.z, 0.0f) + std::max(center.z - max().z, 0.0f);

    return (ex < radius) && (ey < radius) && (ez < radius) && (ex * ex + ey * ey + ez * ez < radius * radius);
}

void AABB::encapsulate(const AABB &bounds) {
    encapsulate(bounds.center_ - bounds.extents_);
    encapsulate(bounds.center_ + bounds.extents_);
}

void AABB::encapsulate(const Vec3& point) {
    set_min_max(Vec3::min(min(), point), Vec3::max(max(), point));
}

std::ostream& operator<<(std::ostream& stream, const AABB& aabb) {
    stream << "AABB(" << aabb.min() << ", " << aabb.max() << ")";
    return stream;
}

}
