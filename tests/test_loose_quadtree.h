#pragma once


#include "../simulant/nodes/geoms/loose_quadtree.h"
#include "../simulant/frustum.h"

namespace smlt {

struct QuadtreeTestTreeData {};
struct QuadtreeTestNodeData {};

typedef Quadtree<QuadtreeTestTreeData, QuadtreeTestNodeData> TestQuadtree;

class LooseQuadtreeTests : public smlt::test::TestCase {
public:
    void test_calc_base() {
        assert_equal(TestQuadtree::calc_base(0), 0u);
        assert_equal(TestQuadtree::calc_base(1), 1u);
        assert_equal(TestQuadtree::calc_base(2), 5u);
        assert_equal(TestQuadtree::calc_base(3), 21u); // 16 + 4 + 1
        assert_equal(TestQuadtree::calc_base(4), 85u);
    }

    void test_child_indexes() {
        TestQuadtree::Node root;
        root.level = 0;
        root.grid[0] = root.grid[1] = 0;

        auto indexes = TestQuadtree::child_indexes(root);

        assert_equal(indexes.size(), 4u);

        assert_equal(indexes[0], 1u);
        assert_equal(indexes[1], 2u);
        assert_equal(indexes[2], 3u);
        assert_equal(indexes[3], 4u);

        root.level = 1;
        indexes = TestQuadtree::child_indexes(root);

        assert_equal(indexes.size(), 4u);

        assert_equal(indexes[0], 5u);
    }

    void test_find_destination_for_triangle() {
        AABB bounds(smlt::Vec3(), 10.0f);

        TestQuadtree Quadtree(bounds, 2); // Create a two level Quadtree for simplicity

        smlt::Vec3 vertices[3];
        vertices[0] = smlt::Vec3(-4.5, 4.5, -4.5);
        vertices[1] = smlt::Vec3(-4.6, 4.5, -4.6);
        vertices[2] = smlt::Vec3(-4.5, 4.5, -4.6);

        auto node = Quadtree.find_destination_for_triangle(vertices);

        assert_equal(+node->grid[0], 0);
        assert_equal(+node->grid[1], 0);

        vertices[0] = smlt::Vec3(-4.5, -4.5, -4.5);
        vertices[1] = smlt::Vec3(-4.6, -4.5, -4.6);
        vertices[2] = smlt::Vec3(-4.5, -4.5, -4.6);

        node = Quadtree.find_destination_for_triangle(vertices);

        assert_equal(+node->grid[0], 0);
        assert_equal(+node->grid[1], 0);
    }

    void test_traverse_visible() {
        AABB bounds(smlt::Vec3(), 10.0f);
        TestQuadtree Quadtree(bounds, 2); // Create a two level Quadtree for simplicity

        Mat4 projection = Mat4::as_look_at(Vec3(0, 0, -4), Vec3(0, 0, -10), Vec3(0, 1, 0));

        Frustum identity;
        identity.build(&projection);

        auto cb = [=](TestQuadtree::Node* node) {
            assert_equal(+node->grid[1], 0);
        };

        Quadtree.traverse_visible(identity, cb);

    }

    void test_traverse() {

    }

    void test_leaf_nodes_set_correctly() {

    }
};

}
