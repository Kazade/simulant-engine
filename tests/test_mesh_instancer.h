#pragma once

#include <simulant/test.h>
#include <simulant/simulant.h>

namespace {

using namespace smlt;

class MeshInstancerTests : public smlt::test::SimulantTestCase {
public:

    void set_up() {
        smlt::test::SimulantTestCase::set_up();

        stage_ = scene->create_child<smlt::Stage>();
        mesh_ = scene->assets->create_mesh_as_cube_with_submesh_per_face(1.0f);
    }

    void test_mesh_instancer_creation() {
        scene->create_child<smlt::MeshInstancer>(mesh_);
        assert_equal(scene->count_nodes_by_type<smlt::MeshInstancer>(), 1u);

        scene->create_child<smlt::MeshInstancer>(mesh_);
        assert_equal(scene->count_nodes_by_type<smlt::MeshInstancer>(), 2u);
    }

    void test_mesh_instancer_destruction() {
        auto instancer = scene->create_child<MeshInstancer>(mesh_);
        assert_equal(scene->count_nodes_by_type<MeshInstancer>(), 1u);

        instancer->destroy();
        application->run_frame();

        assert_equal(scene->count_nodes_by_type<MeshInstancer>(), 0u);
    }

    void test_find_mesh_instancer() {
        auto instancer = scene->create_child<MeshInstancer>(mesh_);
        assert_true(instancer->name().empty());

        instancer->set_name("instancer");

        auto found = scene->find_descendent_with_name("instancer");
        assert_true(found);
        assert_equal(found, instancer);
    }

    void test_spawn_instances_changes_aabb() {
        auto instancer = scene->create_child<MeshInstancer>(mesh_);
        assert_true(instancer->aabb().has_zero_area());

        instancer->create_mesh_instance(smlt::Vec3());

        assert_false(instancer->aabb().has_zero_area());

        /* Spawned around 0,0,0 - the two aabbs should match */
        assert_equal(mesh_->aabb(), instancer->aabb());
    }

    void test_spawn_instances_updates_renderables() {
        auto instancer = scene->create_child<MeshInstancer>(mesh_);

        auto camera = scene->create_child<smlt::Camera>();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        Viewport viewport;
        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        /* Nothing there yet! */
        assert_equal(queue.renderable_count(), 0u);

        instancer->create_mesh_instance(smlt::Vec3());
        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        assert_equal(queue.renderable_count(), mesh_->submesh_count());
        queue.clear();

        instancer->create_mesh_instance(smlt::Vec3(100));
        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        assert_equal(queue.renderable_count(), mesh_->submesh_count() * 2);
    }

    void test_hidden_instances_arent_in_renderables() {
        Viewport viewport;

        auto instancer = scene->create_child<MeshInstancer>(mesh_);

        auto camera = scene->create_child<smlt::Camera>();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        /* Nothing there yet! */
        assert_equal(queue.renderable_count(), 0u);

        auto iid = instancer->create_mesh_instance(smlt::Vec3());
        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        assert_equal(queue.renderable_count(), mesh_->submesh_count());
        queue.clear();

        /* Hide the only instance */
        instancer->hide_mesh_instance(iid);
        instancer->generate_renderables(&queue, camera, &viewport,DETAIL_LEVEL_NEAREST);

        /* Not returned */
        assert_equal(queue.renderable_count(), 0u);
    }

    void test_set_mesh_changes_aabb() {
        /* Create a bigger cube */
        auto mesh2 = scene->assets->create_mesh_as_cube_with_submesh_per_face(2.0f);

        assert_not_equal(mesh_->aabb(), mesh2->aabb());

        auto instancer = scene->create_child<MeshInstancer>(mesh_);
        instancer->create_mesh_instance(Vec3());

        assert_equal(instancer->aabb(), mesh_->aabb());

        instancer->set_mesh(mesh2);

        assert_equal(instancer->aabb(), mesh2->aabb());
    }

    void test_null_mesh_returns_no_renderables() {
        Viewport viewport;
        auto instancer = scene->create_child<MeshInstancer>(nullptr);
        instancer->create_mesh_instance(Vec3());

        auto camera = scene->create_child<smlt::Camera>();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);
        assert_equal(queue.renderable_count(), 0u);
    }

    void test_mesh_instance_id_different() {
        auto instancer = scene->create_child<MeshInstancer>(mesh_);
        auto iid1 = instancer->create_mesh_instance(Vec3());
        auto iid2 = instancer->create_mesh_instance(Vec3());
        auto iid3 = instancer->create_mesh_instance(Vec3());

        assert_true(iid1);
        assert_true(iid2);
        assert_true(iid3);

        assert_not_equal(iid1, iid2);
        assert_not_equal(iid1, iid3);
        assert_not_equal(iid2, iid3);
    }

    void test_transform_is_relative() {
        Viewport viewport;
        auto instancer = scene->create_child<MeshInstancer>(mesh_);
        instancer->create_mesh_instance(Vec3());
        assert_equal(instancer->transformed_aabb().centre(), smlt::Vec3(0, 0, 0));

        instancer->transform->set_translation(Vec3(10, 0, 0));
        assert_equal(instancer->transformed_aabb().centre(), smlt::Vec3(10, 0, 0));

        auto camera = scene->create_child<smlt::Camera>();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        instancer->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        assert_close(queue.renderable(0)->final_transformation[12], 10.0f, 0.0001f);
    }

private:
    smlt::StagePtr stage_;
    smlt::MeshPtr mesh_;
};

}
