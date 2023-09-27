#pragma once

#include <cmath>
#include <array>
#include "vec3.h"

namespace smlt {

class VertexData;

enum AABBCorner {
    AABB_CORNER_NEG_X_NEG_Y_NEG_Z = 0,
    AABB_CORNER_POS_X_NEG_Y_NEG_Z = 1,
    AABB_CORNER_POS_X_NEG_Y_POS_Z = 2,
    AABB_CORNER_NEG_X_NEG_Y_POS_Z = 3,
    AABB_CORNER_NEG_X_POS_Y_NEG_Z = 4,
    AABB_CORNER_POS_X_POS_Y_NEG_Z = 5,
    AABB_CORNER_POS_X_POS_Y_POS_Z = 6,
    AABB_CORNER_NEG_X_POS_Y_POS_Z = 7
};

class AABB {
    Vec3 center_;
    Vec3 extents_;

public:
    const static AABB ZERO;

    void set_min_max(const Vec3& min, const Vec3& max) {
        extents_ = (max - min) * 0.5f;
        center_ = min + extents_;
    }

    Vec3 min() const {
        return center_ - extents_;
    }

    Vec3 max() const {
        return center_ + extents_;
    }

    AABB() = default;

    AABB(const Vec3& center, const Vec3& extents):
        center_(center), extents_(extents) {}

    AABB(const Vec3& centre, float width);
    AABB(const Vec3& centre, float xsize, float ysize, float zsize);
    AABB(const Vec3* vertices, const std::size_t count);    
    AABB(const VertexData& vertex_data);

    float width() const {
        return extents_.x * 2.0f;
    }

    float height() const {
        return extents_.y * 2.0f;
    }

    float depth() const  {
        return extents_.z * 2.0f;
    }

    const Vec3 dimensions() const {
        return Vec3(width(), height(), depth());
    }

    float max_dimension() const {
        return fast_max(width(), fast_max(height(), depth()));
    }

    float min_dimension() const {
        return fast_min(width(), fast_min(height(), depth()));
    }

    bool intersects_aabb(const AABB& other) const;
    bool intersects_sphere(const smlt::Vec3& center, float radius) const;

    Vec3 centre() const {
        return center_;
    }

    bool has_zero_area() const {
        /*
         * Returns True if the AABB has two or more zero dimensions
         */
        bool empty_x = width() == 0.0f;
        bool empty_y = height() == 0.0f;
        bool empty_z = depth() == 0.0f;

        return (empty_x && empty_y) || (empty_x && empty_z) || (empty_y && empty_z);
    }

    bool contains_point(const Vec3& p) const {
        // FIXME: Use extents_ directly
        if(p.x >= min().x && p.x <= max().x &&
           p.y >= min().y && p.y <= max().y &&
           p.z >= min().z && p.z <= max().z) {
            return true;
        }

        return false;
    }

    bool contains_points(const Vec3* vertices, std::size_t count) const {
        for(std::size_t i = 0; i < count; ++i) {
            if(!contains_point(vertices[i])) {
                return false;
            }
        }

        return true;
    }

    bool contains_points(const std::vector<Vec3>& points) const {
        return contains_points(&points[0], points.size());
    }

    std::array<Vec3, 8> corners() const {
        return {
            center_ + Vec3(-extents_.x, -extents_.y, -extents_.z),
            center_ + Vec3( extents_.x, -extents_.y, -extents_.z),
            center_ + Vec3( extents_.x, -extents_.y,  extents_.z),
            center_ + Vec3(-extents_.x, -extents_.y,  extents_.z),
            center_ + Vec3(-extents_.x,  extents_.y, -extents_.z),
            center_ + Vec3( extents_.x,  extents_.y, -extents_.z),
            center_ + Vec3( extents_.x,  extents_.y,  extents_.z),
            center_ + Vec3(-extents_.x,  extents_.y,  extents_.z),
        };
    }

    void encapsulate(const AABB& other);
    void encapsulate(const Vec3& point);

    bool operator==(const AABB& rhs) const {
        return min().equals(rhs.min()) && max().equals(rhs.max());
    }

    bool operator!=(const AABB& rhs) const {
        return !(*this == rhs);
    }

    friend std::ostream& operator<<(std::ostream& stream, const AABB& aabb);
};

std::ostream& operator<<(std::ostream& stream, const AABB& aabb);

}
