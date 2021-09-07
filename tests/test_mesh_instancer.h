#pragma once

#include <simulant/test.h>

namespace {

using namespace smlt;

class MeshInstancerTests : public smlt::test::SimulantTestCase {
public:

    void set_up() {
        smlt::test::SimulantTestCase::set_up();

        stage_ = window->new_stage();
        mesh_ = stage_->assets->new_mesh_as_cube_with_submesh_per_face(1.0f);
    }

    void test_mesh_instancer_creation() {
        stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 1u);

        stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 2u);
    }

    void test_mesh_instancer_destruction() {
        bool ret = stage_->destroy_mesh_instancer(0);
        assert_false(ret);

        ret = stage_->destroy_mesh_instancer(1);
        assert_false(ret);

        auto instancer = stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 1u);

        ret = stage_->destroy_mesh_instancer(instancer);
        assert_true(ret);

        window->run_frame();

        instancer = stage_->new_mesh_instancer(mesh_);
        assert_equal(stage_->mesh_instancer_count(), 1u);

        instancer->destroy();
        window->run_frame();

        assert_equal(stage_->mesh_instancer_count(), 0u);
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
        auto instancer = stage_->new_mesh_instancer(mesh_);

        auto camera = stage_->new_camera();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        /* Nothing there yet! */
        assert_equal(queue.renderable_count(), 0u);

        instancer->new_mesh_instance(smlt::Vec3());
        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        assert_equal(queue.renderable_count(), mesh_->submesh_count());
        queue.clear();

        instancer->new_mesh_instance(smlt::Vec3(100));
        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        assert_equal(queue.renderable_count(), mesh_->submesh_count() * 2);
    }

    void test_hidden_instances_arent_in_renderables() {
        auto instancer = stage_->new_mesh_instancer(mesh_);

        auto camera = stage_->new_camera();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        /* Nothing there yet! */
        assert_equal(queue.renderable_count(), 0u);

        auto iid = instancer->new_mesh_instance(smlt::Vec3());
        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        assert_equal(queue.renderable_count(), mesh_->submesh_count());
        queue.clear();

        /* Hide the only instance */
        instancer->hide_mesh_instance(iid);
        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        /* Not returned */
        assert_equal(queue.renderable_count(), 0u);
    }

    void test_set_mesh_changes_aabb() {
        /* Create a bigger cube */
        auto mesh2 = stage_->assets->new_mesh_as_cube_with_submesh_per_face(2.0f);

        assert_not_equal(mesh_->aabb(), mesh2->aabb());

        auto instancer = stage_->new_mesh_instancer(mesh_);
        instancer->new_mesh_instance(Vec3());

        assert_equal(instancer->aabb(), mesh_->aabb());

        instancer->set_mesh(mesh2);

        assert_equal(instancer->aabb(), mesh2->aabb());
    }

    void test_null_mesh_returns_no_renderables() {
        auto instancer = stage_->new_mesh_instancer(smlt::MeshID());
        instancer->new_mesh_instance(Vec3());

        auto camera = stage_->new_camera();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);
        assert_equal(queue.renderable_count(), 0u);
    }

    void test_mesh_instance_id_different() {
        auto instancer = stage_->new_mesh_instancer(mesh_);
        auto iid1 = instancer->new_mesh_instance(Vec3());
        auto iid2 = instancer->new_mesh_instance(Vec3());
        auto iid3 = instancer->new_mesh_instance(Vec3());

        assert_true(iid1);
        assert_true(iid2);
        assert_true(iid3);

        assert_not_equal(iid1, iid2);
        assert_not_equal(iid1, iid3);
        assert_not_equal(iid2, iid3);
    }

    void test_transform_is_relative() {
        auto instancer = stage_->new_mesh_instancer(mesh_);
        instancer->new_mesh_instance(Vec3());
        assert_equal(instancer->transformed_aabb().centre(), smlt::Vec3(0, 0, 0));

        instancer->move_to(10, 0, 0);
        assert_equal(instancer->transformed_aabb().centre(), smlt::Vec3(10, 0, 0));

        auto camera = stage_->new_camera();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->_get_renderables(&queue, camera, DETAIL_LEVEL_NEAREST);

        assert_close(queue.renderable(0)->final_transformation[12], 10.0f, 0.0001f);
    }

private:
    smlt::StagePtr stage_;
    smlt::MeshPtr mesh_;
};

}
