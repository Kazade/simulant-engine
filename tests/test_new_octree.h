#pragma once

#include <memory>
#include "global.h"
#include "kglt/kglt.h"
#include "kglt/partitioners/impl/octree.h"


class NewOctreeTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        stage_id_ = window->new_stage();
        octree_.reset(new kglt::impl::Octree(window->stage(stage_id_)));
    }

    void test_initial_insert() {
        auto octree_node = octree_->insert_actor(actor_id_);

        assert_false(octree_node->has_children());
        assert_true(octree_node->is_root());
        assert_equal(0, octree_node->siblings().size());
        assert_equal(0, octree_node->immediate_children().size());
        assert_is_null(octree_node->parent());
        assert_close(10.0f, octree_node->diameter(), 0.001);
        assert_true(octree_node->is_empty());

        assert_equal(octree_node, octree_->locate_actor(actor_id_));

        octree_->remove_actor(actor_id_);

        octree_->prune_empty_nodes();
        assert_equal(octree_node, octree_->locate_actor(actor_id_));

        octree_->prune_empty_nodes();
        assert_is_null(octree_->locate_actor(actor_id_));
        assert_true(octree_->is_empty());
    }

    void tear_down() {
        window->delete_stage(stage_id_);
        octree_.reset();
        KGLTTestCase::tear_down();
    }

private:
    std::unique_ptr<kglt::impl::Octree> octree_;
    kglt::StageID stage_id_;
    kglt::ActorID actor_id_;
};
