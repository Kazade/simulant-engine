#pragma once

#include <memory>
#include "global.h"
#include "kglt/kglt.h"
#include "kglt/partitioners/impl/octree.h"

struct NodeData {
    uint32_t poly_count = 0;

    bool is_empty() const {
        return poly_count == 0;
    }
};

struct Object : public kglt::Boundable {
    Object(const kglt::Vec3& centre, float width):
        aabb(centre, width) {}

    kglt::AABB aabb;
};

class NewOctreeTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        octree_.reset(new kglt::impl::Octree<NodeData>());
    }

    void test_initial_insert() {
        Object obj(kglt::Vec3(0, 0, 0), 10.0);

        auto octree_node = octree_->insert(&obj);

        assert_false(octree_node->has_children());
        assert_true(octree_node->is_root());
        assert_equal(0, octree_node->siblings().size());
        assert_equal(0, octree_node->immediate_children().size());
        assert_is_null(octree_node->parent());
        assert_close(10.0f, octree_node->diameter(), 0.001);
        assert_true(octree_node->is_empty());

        assert_equal(octree_node, octree_->locate(&obj));

        octree_node->data->poly_count = 1;
        octree_->prune_empty_nodes();
        assert_equal(octree_node, octree_->locate(&obj));

        octree_node->data->poly_count = 0;
        octree_->prune_empty_nodes();
        assert_is_null(octree_->locate(&obj));
        assert_true(octree_->is_empty());
    }

    void tear_down() {
        octree_.reset();
        KGLTTestCase::tear_down();
    }

private:
    std::unique_ptr<kglt::impl::Octree<NodeData>> octree_;

};
