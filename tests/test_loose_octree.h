#pragma once

#include "global.h"
#include "../simulant/nodes/geoms/loose_octree.h"
#include "../simulant/frustum.h"

namespace smlt {

struct TestTreeData {};
struct TestNodeData {};

typedef Octree<TestTreeData, TestNodeData> TestOctree;

class LooseOctreeTests : public TestCase {
public:
    void test_calc_base() {
        assert_equal(TestOctree::calc_base(0), 0u);
        assert_equal(TestOctree::calc_base(1), 1u);
        assert_equal(TestOctree::calc_base(2), 9u);
        assert_equal(TestOctree::calc_base(3), 73u);
        assert_equal(TestOctree::calc_base(4), 585u);
    }

    void test_child_indexes() {
        TestOctree::Node root;
        root.level = 0;
        root.grid[0] = root.grid[1] = root.grid[2] = 0;

        auto indexes = TestOctree::child_indexes(root);

        assert_equal(indexes.size(), 8u);

        assert_equal(indexes[0], 1u);
        assert_equal(indexes[1], 2u);
        assert_equal(indexes[2], 3u);
        assert_equal(indexes[3], 4u);

        assert_equal(indexes[4], 5u);
        assert_equal(indexes[5], 6u);
        assert_equal(indexes[6], 7u);
        assert_equal(indexes[7], 8u);

        root.level = 1;
        indexes = TestOctree::child_indexes(root);

        assert_equal(indexes.size(), 8u);

        assert_equal(indexes[0], 9u);
    }

    void test_find_destination_for_triangle() {
        AABB bounds(smlt::Vec3(), 10.0f);

        TestOctree octree(bounds, 2); // Create a two level octree for simplicity

        smlt::Vec3 vertices[3];
        vertices[0] = smlt::Vec3(-4.5, 4.5, -4.5);
        vertices[1] = smlt::Vec3(-4.6, 4.5, -4.6);
        vertices[2] = smlt::Vec3(-4.5, 4.5, -4.6);

        auto node = octree.find_destination_for_triangle(vertices);

        assert_equal(+node->grid[0], 0);
        assert_equal(+node->grid[1], 1);
        assert_equal(+node->grid[2], 0);

        vertices[0] = smlt::Vec3(-4.5, -4.5, -4.5);
        vertices[1] = smlt::Vec3(-4.6, -4.5, -4.6);
        vertices[2] = smlt::Vec3(-4.5, -4.5, -4.6);

        node = octree.find_destination_for_triangle(vertices);

        assert_equal(+node->grid[0], 0);
        assert_equal(+node->grid[1], 0);
        assert_equal(+node->grid[2], 0);
    }

    void test_traverse_visible() {
        AABB bounds(smlt::Vec3(), 10.0f);
        TestOctree octree(bounds, 2); // Create a two level octree for simplicity

        Mat4 projection = Mat4::as_look_at(Vec3(0, 0, -4), Vec3(0, 0, -10), Vec3(0, 1, 0));

        Frustum identity;
        identity.build(&projection);

        auto cb = [=](TestOctree::Node* node) {
            // Should only traverse nodes in the negative-Z
            assert_equal(+node->grid[2], 0);
        };

        octree.traverse_visible(identity, cb);

    }

    void test_traverse() {

    }

    void test_leaf_nodes_set_correctly() {

    }
};

}
