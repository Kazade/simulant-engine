#pragma once

#include <cstdint>
#include "../../math/aabb.h"
#include "../../frustum.h"
#include "../../generic/check_signature.h"

namespace smlt {

namespace _loose_octree {
    static constexpr int64_t ipow(int64_t base, int exp, int64_t result = 1) {
      return exp < 1 ? result : ipow(base*base, exp/2, (exp % 2) ? result*base : result);
    }
}

/*
 * Fast loose-octree implementation with O(1) insertions, built on a linear
 * array for good cache locality.
 *
 * TreeData is a structure which contains data needed across the whole tree,
 * this may usually contain the vertex data for the mesh being inserted.
 *
 * NodeData is a structure for storing data on the individual nodes, this is usually
 * index data for the mesh.
 */

typedef uint8_t Level;
typedef uint16_t GridCoord;

template<typename TreeData, typename NodeData>
class Octree {
public:
    struct Node {
        Level level = 0;
        GridCoord grid[3] = {0, 0, 0};
        NodeData* data = nullptr;
        uint32_t child_indexes[8];

        Vec3 centre;
        float size;
    };

    using TraverseCallback = void(Octree::Node*);

    typedef TreeData tree_data_type;
    typedef NodeData node_data_type;

    Octree(const AABB& bounds, uint8_t max_level_count=4, TreeData* tree_data=nullptr):
        tree_data_(tree_data),
        root_width_(bounds.max_dimension()),
        bounds_(bounds),
        centre_(bounds_.centre()) {

        /* Make sure the bounds are square */
        float maxd = root_width_;
        auto halfd = maxd / 2.0f;

        auto half = Vec3(halfd, halfd, halfd);
        bounds_.set_min(centre_ - half);
        bounds_.set_max(centre_ + half);

        /* Grow the tree to whatever size we passed in */
        grow(max_level_count);
    }

    virtual ~Octree() {
        /* Make sure we delete the node data */
        for(auto& node: nodes_) {
            delete node.data;
            node.data = nullptr;
        }
    }

    bool is_leaf(Node& node) const {
        return node.level == (levels_ - 1);
    }

    Octree::Node* find_destination_for_sphere(const Vec3& centre, float radius) {
        auto diameter = radius * 2;
        auto level_and_node_width = level_for_width(diameter);

        /* Calculate the cell index to insert the sphere */
        auto half_width = root_width_ * 0.5f;

        assert(centre.x <= bounds_.max().x && centre.x >= bounds_.min().x);
        assert(centre.y <= bounds_.max().y && centre.y >= bounds_.min().y);
        assert(centre.z <= bounds_.max().z && centre.z >= bounds_.min().z);

        auto x = (GridCoord) ((centre.x + half_width - centre_.x) / level_and_node_width.second);
        auto y = (GridCoord) ((centre.y + half_width - centre_.y) / level_and_node_width.second);
        auto z = (GridCoord) ((centre.z + half_width - centre_.z) / level_and_node_width.second);

        assert(x >= 0);
        assert(y >= 0);
        assert(z >= 0);

        auto level_width = _loose_octree::ipow(2, level_and_node_width.first);
        assert(x <= level_width);
        assert(y <= level_width);
        assert(z <= level_width);

        /* This handles the case that the center was on the cusp of the cell
         * in which case the above calculation will result in an out of range
         * index */
        x = std::min<GridCoord>(x, level_width - 1);
        y = std::min<GridCoord>(y, level_width - 1);
        z = std::min<GridCoord>(z, level_width - 1);

        auto idx = calc_index(level_and_node_width.first, x, y, z);
        assert(idx < nodes_.size());

        return &nodes_[idx];
    }

    Octree::Node* find_destination_for_triangle(const Vec3* vertices) {
        /*
         * Return the node that this triangle should be
         * inserted into (the actual insertion won't happen as that's implementation specific
         * depending on the NodeData).
         */

        AABB bounds(vertices, 3);
        auto centre = bounds.centre();
        auto radius = bounds.max_dimension() / 2.0f;
        return find_destination_for_sphere(centre, radius);
    }

    void traverse(std::function<void (Octree::Node*)> cb) {
        if(nodes_.empty()) {
            return;
        }

        std::function<void (Octree::Node&)> visitor = [&](Octree::Node& node) {
            cb(&node);

            if(!is_leaf(node)) {
                for(auto child: node.child_indexes) {
                    assert(child < nodes_.size());
                    visitor(nodes_[child]);
                }
            }
        };

        visitor(nodes_[0]);
    }

    template<typename Callback>
    void traverse_visible(const Frustum& frustum, const Callback& cb) {
        check_signature<Callback, TraverseCallback>();

        if(nodes_.empty()) {
            return;
        }

        _visible_visitor(frustum, cb, nodes_[0]);
    }

    AABB bounds() const { return bounds_; }

private:
    template<typename Callback>
    void _visible_visitor(const Frustum& frustum, const Callback& callback, Octree::Node& node) {
        if(frustum.intersects_cube(node.centre, node.size * 2.0f)) {
            callback(&node);

            if(!is_leaf(node)) {
                for(auto child: node.child_indexes) {
                    assert(child < nodes_.size());
                    _visible_visitor(frustum, callback, nodes_[child]);
                }
            }
        }
    }

    std::pair<Level, float> level_for_width(float obj_width) {
        /*
         * Given the diameter of the object
         * this returns the level the object will fit, and the node width at that level
         *
         * I *know* there's a non-iterative way to calculate this, so if you know, let me know!
         */

        assert(obj_width <= root_width_);

        if(obj_width > root_width_) {
            // Should never happen, but this is the safest thing to do if it does
            return std::make_pair(0, root_width_);
        }

        auto node_width = root_width_;
        Level depth = 0;

        while(node_width >= obj_width) {
            ++depth;
            node_width *= 0.5f;

            if(depth == levels_) {
                break;
            }
        }

        assert(depth >= 1);

        // Off-by-one
        return std::make_pair(depth - 1, node_width * 2);
    }

    void calc_bounds(Octree::Node& node) const {
        auto grid_width = _loose_octree::ipow(2, node.level);
        auto cell_width = bounds_.max_dimension() / grid_width;

        Vec3 min(node.grid[0] * cell_width, node.grid[1] * cell_width, node.grid[2] * cell_width);
        min += bounds_.min();

        float hw = cell_width * 0.5f;
        node.centre = min + Vec3(hw, hw, hw);
        node.size = cell_width;
    }

    static uint32_t calc_base(uint8_t level) {
        if(level == 0) return 0;
        if(level == 1) return 1;
        return calc_base(level - 1) + _loose_octree::ipow(_loose_octree::ipow(2, level - 1), 3);
    }

    static uint32_t calc_index(Level k, GridCoord x, GridCoord y, GridCoord z) {
        auto level_base = calc_base(k);
        auto level_width = _loose_octree::ipow(2, k);

        assert(x < level_width);
        assert(y < level_width);
        assert(z < level_width);

        auto idx = x + level_width * y + level_width * level_width * z;
        return level_base + idx;
    }

    static void calc_child_indexes(Octree::Node& node) {
        uint8_t count = 0;

        for(uint32_t z = 0; z <= 1; ++z) {
            for(uint32_t y = 0; y <= 1; ++y) {
                for(uint32_t x = 0; x <= 1; ++x) {
                    node.child_indexes[count++] = calc_index(
                        node.level + 1,
                        2 * node.grid[0] + x,
                        2 * node.grid[1] + y,
                        2 * node.grid[2] + z
                    );
                }
            }
        }
    }

    void grow(uint8_t required_levels) {

        auto required_nodes = 0;
        for(uint8_t i = 0; i < required_levels; ++i) {
            auto nodes_across = _loose_octree::ipow(2, i);
            required_nodes += _loose_octree::ipow(nodes_across, 3);
        }

        levels_ = required_levels;
        nodes_.resize(required_nodes, Octree::Node());

        for(auto k = 0; k < levels_; ++k) {
            auto nodes_across = _loose_octree::ipow(2, k);

            for(GridCoord z = 0; z < nodes_across; ++z) {
                for(GridCoord y = 0; y < nodes_across; ++y) {
                    for(GridCoord x = 0; x < nodes_across; ++x) {
                        auto idx = calc_index(k, x, y, z);
                        assert(idx < nodes_.size());

                        auto& new_node = nodes_[idx];

                        assert(!new_node.data);

                        new_node.grid[0] = x;
                        new_node.grid[1] = y;
                        new_node.grid[2] = z;
                        new_node.level = k;
                        new_node.data = new NodeData();

                        calc_child_indexes(new_node);
                        calc_bounds(new_node);
                    }
                }
            }
        }
    }

    TreeData* tree_data_ = nullptr;

    float root_width_;
    AABB bounds_;
    Vec3 centre_;

    Level levels_ = 0;
    std::vector<Octree::Node> nodes_;

    friend class LooseOctreeTests;
};

}
