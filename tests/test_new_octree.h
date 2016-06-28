#pragma once

#include <memory>
#include "global.h"
#include "kglt/kglt.h"
#include "kglt/partitioners/impl/octree.h"


class NewOctreeTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        stage_id_ = window->new_stage(kglt::PARTITIONER_NULL);
        stage_ = window->stage(stage_id_);

        /* We have a loose octree, so nodes can contain objects which are twice their size
            by setting the cube size to 20, we should expect that a default node created for it
            will have a diameter of 10.0f;
        */
        actor_id_ = stage_->new_actor_with_mesh(stage_->new_mesh_as_cube(32.0));
        octree_.reset(new kglt::impl::Octree(stage_.get().get()));
    }

    void test_octreenode_contains() {
        octree_->insert_actor(actor_id_); // Dimension is now 16,16,16

        assert_true(octree_->get_root()->contains(kglt::Vec3(0, 0, 0)));
        assert_true(octree_->get_root()->contains(kglt::Vec3(5, 0, 0)));
        assert_false(octree_->get_root()->contains(kglt::Vec3(9, 0, 0)));
        assert_true(octree_->get_root()->contains(kglt::Vec3(8, 8, 0)));
        assert_false(octree_->get_root()->contains(kglt::Vec3(8, 8.1, 0)));
    }

    void test_initial_insert() {
        auto octree_node = octree_->insert_actor(actor_id_);

        assert_false(octree_node->has_children());
        assert_true(octree_node->is_root());
        assert_equal(0, octree_node->children().size());
        assert_is_null(octree_node->parent());
        assert_equal(16, octree_node->diameter());
        assert_false(octree_node->is_empty());

        assert_equal(octree_node, octree_->locate_actor(actor_id_));

        octree_->remove_actor(actor_id_);

        assert_is_null(octree_->locate_actor(actor_id_));
        assert_true(octree_->is_empty());
        assert_equal(octree_->node_count(), 0);
    }

    void test_octree_growth() {
        auto octree_node = octree_->insert_actor(actor_id_);
        assert_equal(16, octree_node->diameter());

        // Too big for this node, time to grow!
        auto actor_2 = stage_->new_actor_with_mesh(stage_->new_mesh_as_cube(40.0f));

        auto new_root = octree_->insert_actor(actor_2);

        assert_equal(0, new_root->level());
        assert_equal(1, octree_node->level());
        assert_equal(32, new_root->diameter());

        // FIXME: If we redistribute the existing actors, this node should disappear
        // and all actors will be in the root.
        assert_equal(16, octree_node->diameter());
    }

    void test_octree_shrinks() {
        auto octree_node = octree_->insert_actor(actor_id_);
        assert_equal(16, octree_node->diameter());

        // Too big for this node, time to grow!
        auto actor_2 = stage_->new_actor_with_mesh(stage_->new_mesh_as_cube(40.0f));

        auto new_root = octree_->insert_actor(actor_2);

        assert_equal(0, new_root->level());
        assert_equal(1, octree_node->level());
        assert_equal(32, new_root->diameter());

        // FIXME: If we redistribute the existing actors, this node should disappear
        // and all actors will be in the root.
        assert_equal(16, octree_node->diameter());

        octree_->remove_actor(actor_2);

        assert_equal(16, octree_->get_root()->diameter());
        assert_equal(1, octree_->node_count());
    }

    void test_child_centres() {
        auto octree_node = octree_->insert_actor(actor_id_);

        std::vector<kglt::Vec3> expected = {
            kglt::Vec3(-4.0, -4.0, -4.0),
            kglt::Vec3(-4.0,  4.0, -4.0),
            kglt::Vec3( 4.0,  4.0, -4.0),
            kglt::Vec3( 4.0, -4.0, -4.0),
            kglt::Vec3(-4.0, -4.0,  4.0),
            kglt::Vec3(-4.0,  4.0,  4.0),
            kglt::Vec3( 4.0,  4.0,  4.0),
            kglt::Vec3( 4.0, -4.0,  4.0),
        };

        assert_items_equal(expected, octree_node->child_centres());

    }

    void test_insert_empty_aabb() {
        auto blank_actor = stage_->new_actor();
        kglt::impl::OctreeNode* node = octree_->insert_actor(blank_actor);
        assert_equal(1, node->diameter()); // 1 is the minimum node size
    }

    void test_splitting_nodes() {
        auto split_predicate = [](kglt::impl::OctreeNode* node) -> bool {
            return node->data->actor_ids_.size() > 2;
        };

        auto octree = std::make_shared<kglt::impl::Octree>(stage_.get().get(), split_predicate);

        octree->insert_actor(actor_id_);
        assert_equal(octree->node_count(), 1);

        auto second = stage_->new_actor_with_mesh(stage_->new_mesh_as_cube(1.0));
        octree->insert_actor(second);
        assert_equal(octree->node_count(), 1);

        auto third = stage_->new_actor_with_mesh(stage_->new_mesh_as_cube(1.0));
        auto new_node = octree->insert_actor(third);
        assert_equal(octree->node_count(), 2);
        assert_equal(new_node->parent(), octree->get_root());
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

        point = kglt::Vec3(-2.0, 1, 1);

        ret = octree_->find_node_centre_for_point(1, point);
        expected = kglt::Vec3(-half_root / 2, half_root / 2, half_root / 2);
        assert_equal(expected, ret);

        ret = octree_->find_node_centre_for_point(2, point);
        expected = kglt::Vec3(-quarter_root / 2, quarter_root / 2, quarter_root / 2);
        assert_equal(expected, ret);
    }

    void test_calculate_level() {

    }

    void test_find_best_existing_node() {
        octree_->insert_actor(actor_id_);

        kglt::AABB aabb;
        aabb.min = kglt::Vec3(-1, -1, -1);
        aabb.max = kglt::Vec3(1, 1, 1);

        auto ret = octree_->find_best_existing_node(aabb);

        assert_equal(0, ret.first);
        assert_equal(octree_->levels_.front()->nodes.begin()->first, ret.second);
    }

    void test_node_diameter() {
        octree_->insert_actor(actor_id_);
        assert_equal(16, octree_->node_diameter(0));
        assert_equal(8, octree_->node_diameter(1));
        assert_equal(4, octree_->node_diameter(2));
        assert_equal(2, octree_->node_diameter(3));
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
