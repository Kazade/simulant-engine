#include <limits>
#include "aabb.h"

#include "../vertex_data.h"

namespace smlt {

AABB::AABB(const Vec3 &min, const Vec3 &max) {
    set_min(min);
    set_max(max);

    corners_dirty_ = true;
}

AABB::AABB(const Vec3 &centre, float width) {
    set_min(centre - Vec3(width * 0.5f, width * 0.5f, width * 0.5f));
    set_max(centre + Vec3(width * 0.5f, width * 0.5f, width * 0.5f));

    corners_dirty_ = true;
}

AABB::AABB(const Vec3 &centre, float xsize, float ysize, float zsize) {
    set_min(centre - Vec3(xsize * 0.5f, ysize * 0.5f, zsize * 0.5f));
    set_max(centre + Vec3(xsize * 0.5f, ysize * 0.5f, zsize * 0.5f));

    corners_dirty_ = true;
}

AABB::AABB(const VertexData& vertex_data) {
    if(vertex_data.empty()) {
        set_min(Vec3());
        set_max(Vec3());
        corners_dirty_ = true;
        return;
    }

    float minx = std::numeric_limits<float>::max();
    float miny = std::numeric_limits<float>::max();
    float minz = std::numeric_limits<float>::max();

    float maxx = std::numeric_limits<float>::lowest();
    float maxy = std::numeric_limits<float>::lowest();
    float maxz = std::numeric_limits<float>::lowest();

    for(std::size_t i = 0; i < vertex_data.count(); ++i) {
        auto v = vertex_data.position_nd_at(i);

        if(v.x < minx) minx = v.x;
        if(v.y < miny) miny = v.y;
        if(v.z < minz) minz = v.z;

        if(v.x > maxx) maxx = v.x;
        if(v.y > maxy) maxy = v.y;
        if(v.z > maxz) maxz = v.z;
    }

    set_min(Vec3(minx, miny, minz));
    set_max(Vec3(maxx, maxy, maxz));
    corners_dirty_ = true;
}

AABB::AABB(const Vec3 *vertices, const std::size_t count) {
    if(count == 0) {
        set_min(Vec3());
        set_max(Vec3());
        corners_dirty_ = true;
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
    corners_dirty_ = true;
}

bool AABB::intersects_aabb(const AABB &other) const {
    if (max_.x < other.min_.x) return false;
    if (min_.x > other.max_.x) return false;
    if (max_.y < other.min_.y) return false;
    if (min_.y > other.max_.y) return false;
    if (max_.z < other.min_.z) return false;
    if (min_.z > other.max_.z) return false;

    return true;
}

bool AABB::intersects_sphere(const smlt::Vec3& center, float diameter) const {
    float radius = diameter * 0.5f;

    auto ex = std::max(min_.x - center.x, 0.0f) + std::max(center.x - max_.x, 0.0f);
    auto ey = std::max(min_.y - center.y, 0.0f) + std::max(center.y - max_.y, 0.0f);
    auto ez = std::max(min_.z - center.z, 0.0f) + std::max(center.z - max_.z, 0.0f);

    return (ex < radius) && (ey < radius) && (ez < radius) && (ex * ex + ey * ey + ez * ez < radius * radius);
}

void AABB::encapsulate(const AABB &other) {
    if(other.min_.x < min_.x) min_.x = other.min_.x;
    if(other.min_.y < min_.y) min_.y = other.min_.y;
    if(other.min_.z < min_.z) min_.z = other.min_.z;

    if(other.max_.x > max_.x) max_.x = other.max_.x;
    if(other.max_.y > max_.y) max_.y = other.max_.y;
    if(other.max_.z > max_.z) max_.z = other.max_.z;
}

std::ostream& operator<<(std::ostream& stream, const AABB& aabb) {
    stream << "AABB(" << aabb.min() << ", " << aabb.max() << ")";
    return stream;
}

}
