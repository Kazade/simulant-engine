#pragma once

#include <simulant/test.h>

namespace {

class MeshInstancerTests : public smlt::test::SimulantTestCase {
public:

    void set_up() {
        smlt::test::SimulantTestCase::set_up();

        stage_ = window->new_stage();
        mesh_ = stage_->assets->new_mesh_as_cube_with_submesh_per_face(1.0f);
    }

    void test_mesh_instancer_creation() {
        stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 1);

        stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 2);
    }

    void test_mesh_instancer_destruction() {
        bool ret = stage_->destroy_mesh_instancer(0);
        assert_false(ret);

        ret = stage_->destroy_mesh_instancer(1);
        assert_false(ret);

        auto instancer = stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 1);

        ret = stage_->destroy_mesh_instancer(instancer);
        assert_true(ret);

        instancer = stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 1);

        instancer->destroy();
        assert_equal(stage_->mesh_instancer_count(), 0);
    }

    void test_find_mesh_instancer() {
        auto instancer = stage_->new_mesh_instancer(mesh_);
        assert_true(instancer->name().empty());

        instancer->set_name("instancer");

        auto found = stage_->find_descendent_with_name("instancer");
        assert_true(found);
        assert_equal(found, instancer);
    }

    void test_spawn_instances_changes_aabb() {
        auto instancer = stage_->new_mesh_instancer(mesh_);
        assert_true(instancer->aabb().has_zero_area());

        instancer->new_mesh_instance(smlt::Vec3());

        assert_false(instancer->aabb().has_zero_area());

        /* Spawned around 0,0,0 - the two aabbs should match */
        assert_equal(mesh_->aabb(), instancer->aabb());
    }

    void test_spawn_instances_updates_renderables() {

    }

    void test_hidden_instances_arent_in_renderables() {

    }

    void test_set_mesh_changes_aabb() {

    }

    void test_null_mesh_returns_no_renderables() {

    }

    void test_mesh_instance_id_different() {

    }

private:
    smlt::StagePtr stage_;
    smlt::MeshPtr mesh_;
};

}
