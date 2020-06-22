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
    /* This was originally a basic struct but for performance reasons it's now a class
     * so that we can store things like pre-calculate corners and know they are kept up-to-date
     * with setters */

    Vec3 min_;
    Vec3 max_;

    mutable std::array<Vec3, 8> corners_;
    mutable bool corners_dirty_ = true;

    void rebuild_corners() const {
        corners_[AABB_CORNER_NEG_X_NEG_Y_NEG_Z] = {min_.x, min_.y, min_.z};
        corners_[AABB_CORNER_POS_X_NEG_Y_NEG_Z] = {max_.x, min_.y, min_.z};
        corners_[AABB_CORNER_POS_X_NEG_Y_POS_Z] = {max_.x, min_.y, max_.z};
        corners_[AABB_CORNER_NEG_X_NEG_Y_POS_Z] = {min_.x, min_.y, max_.z};

        corners_[AABB_CORNER_NEG_X_POS_Y_NEG_Z] = {min_.x, max_.y, min_.z};
        corners_[AABB_CORNER_POS_X_POS_Y_NEG_Z] = {max_.x, max_.y, min_.z};
        corners_[AABB_CORNER_POS_X_POS_Y_POS_Z] = {max_.x, max_.y, max_.z};
        corners_[AABB_CORNER_NEG_X_POS_Y_POS_Z] = {min_.x, max_.y, max_.z};

        corners_dirty_ = false;
    }

public:
    void set_min(const Vec3& min) {
        if(min_ != min) {
            min_ = min;
            corners_dirty_ = true;
        }
    }

    void set_min_x(float x) {
        if(x != min_.x) {
            min_.x = x;
            corners_dirty_ = true;
        }
    }

    void set_min_y(float y) {
        if(y != min_.y) {
            min_.y = y;
            corners_dirty_ = true;
        }
    }

    void set_min_z(float z) {
        if(z != min_.z) {
            min_.z = z;
            corners_dirty_ = true;
        }
    }


    void set_max_x(float x) {
        if(x != max_.x) {
            max_.x = x;
            corners_dirty_ = true;
        }
    }

    void set_max_y(float y) {
        if(y != max_.y) {
            max_.y = y;
            corners_dirty_ = true;
        }
    }

    void set_max_z(float z) {
        if(z != max_.z) {
            max_.z = z;
            corners_dirty_ = true;
        }
    }

    void set_max(const Vec3& max) {
        if(max_ != max) {
            max_ = max;
            corners_dirty_ = true;
        }
    }

    const Vec3& min() const { return min_; }
    const Vec3& max() const { return max_; }

    AABB() {
        rebuild_corners();
    }

    AABB(const Vec3& min, const Vec3& max);
    AABB(const Vec3& centre, float width);
    AABB(const Vec3& centre, float xsize, float ysize, float zsize);
    AABB(const Vec3* vertices, const std::size_t count);    
    AABB(const VertexData& vertex_data);

    float width() const {
        return std::abs(max_.x - min_.x);
    }

    float height() const {
        return std::abs(max_.y - min_.y);
    }

    float depth() const  {
        return std::abs(max_.z - min_.z);
    }

    const Vec3 dimensions() const {
        return Vec3(width(), height(), depth());
    }

    float max_dimension() const {
        return std::max(width(), std::max(height(), depth()));
    }

    float min_dimension() const {
        return std::min(width(), std::min(height(), depth()));
    }

    bool intersects_aabb(const AABB& other) const;
    bool intersects_sphere(const smlt::Vec3& center, float radius) const;

    Vec3 centre() const {
        return Vec3(min_) + ((Vec3(max_) - Vec3(min_)) * 0.5f);
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
        if(p.x >= min_.x && p.x <= max_.x &&
           p.y >= min_.y && p.y <= max_.y &&
           p.z >= min_.z && p.z <= max_.z) {
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

    /* NOTE: The corners are recalculated when called, so don't hold onto a reference for long */
    const std::array<Vec3, 8>& corners() const {
        if(corners_dirty_) {
            rebuild_corners();
        }

        return corners_;
    }
};

}
