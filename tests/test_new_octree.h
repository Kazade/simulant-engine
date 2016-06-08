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
        stage_ = window->stage(stage_id_);

        actor_id_ = stage_->new_actor_with_mesh(stage_->new_mesh_as_cube(10.0));
        octree_.reset(new kglt::impl::Octree(stage_));
    }

    void test_initial_insert() {
        auto octree_node = octree_->insert_actor(actor_id_);

        assert_false(octree_node->has_children());
        assert_true(octree_node->is_root());
        assert_equal(0, octree_node->children().size());
        assert_is_null(octree_node->parent());
        assert_close(10.0f, octree_node->diameter(), 0.001);
        assert_false(octree_node->is_empty());

        assert_equal(octree_node, octree_->locate_actor(actor_id_));

        octree_->remove_actor(actor_id_);

        octree_->prune_empty_nodes();
        assert_equal(octree_node, octree_->locate_actor(actor_id_));

        octree_->prune_empty_nodes();
        assert_is_null(octree_->locate_actor(actor_id_));
        assert_true(octree_->is_empty());
    }

    void test_insert_empty_aabb() {
        auto blank_actor = stage_->new_actor();
        assert_raises(kglt::impl::InvalidBoundableInsertion, std::bind(&kglt::impl::Octree::insert_actor, octree_.get(), blank_actor));
    }

    void test_generate_vector_hash() {
        kglt::Vec3 v1(1, 0, 0);
        kglt::Vec3 v2(1.00001, 0, 0);

        assert_equal(octree_->generate_vector_hash(v1), octree_->generate_vector_hash(v2));

        kglt::Vec3 v3(1, 1, 0);

        assert_not_equal(octree_->generate_vector_hash(v1), octree_->generate_vector_hash(v3));
    }

    void test_find_node_centre_for_point() {
        kglt::Vec3 point;
        kglt::impl::NodeLevel level = 0;
        assert_raises(kglt::impl::OutsideBoundsError, std::bind(&kglt::impl::Octree::find_node_centre_for_point, octree_.get(), level, point));

        octree_->insert_actor(actor_id_);

        point = kglt::Vec3(2.0, 1, 1);
        auto ret = octree_->find_node_centre_for_point(0, point);
        assert_equal(kglt::Vec3(), ret); // Root level is centred around 0.0

        auto half_root = octree_->diameter() / 2.0;

        ret = octree_->find_node_centre_for_point(1, point);
        kglt::Vec3 expected(half_root / 2, half_root / 2, half_root / 2);
        assert_equal(expected, ret);

        auto quarter_root = half_root / 2.0;
        ret = octree_->find_node_centre_for_point(2, point);
        expected = kglt::Vec3(quarter_root / 2, quarter_root / 2, quarter_root / 2);
        assert_equal(expected, ret);
    }

    void test_calculate_level() {

    }

    void test_find_best_existing_node() {

    }

    void test_node_diameter() {
        octree_->insert_actor(actor_id_);
        assert_equal(10.0f, octree_->node_diameter(0));
        assert_equal(5.0f, octree_->node_diameter(1));
        assert_equal(2.5f, octree_->node_diameter(2));
        assert_equal(1.25f, octree_->node_diameter(3));
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
    kglt::StagePtr stage_;
};
