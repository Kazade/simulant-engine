#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/partitioners/frustum_partitioner.h"

namespace {

using namespace smlt;

class FrustumPartitionerTests : public test::SimulantTestCase {
public:
    void set_up() override {
        test::SimulantTestCase::set_up();

        stage_ = scene->new_stage();
        box_ = application->shared_assets->new_mesh_as_cube_with_submesh_per_face(1.0f);
    }

    void tear_down() override {
        test::SimulantTestCase::tear_down();

        stage_->destroy();
    }

    void test_only_renders_correct_stage() {
        auto camera = stage_->new_camera();
        camera->move_to(784, 58, -775);

        auto stage2 = scene->new_stage();

        auto a1 = stage_->new_actor_with_mesh(box_);
        auto a2 = stage2->new_actor_with_mesh(box_);

        a1->move_to(791, 58, -810);
        a2->move_to(791, 58, -810);

        std::vector<LightID> lights;
        std::vector<StageNode*> nodes;
        FrustumPartitioner partitioner(stage_);

        partitioner.lights_and_geometry_visible_from(
            camera, lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a2) == nodes.end());

        stage2->destroy();
    }

    void test_visibility() {
        auto camera = stage_->new_camera();
        camera->move_to(784, 58, -775);

        auto a1 = stage_->new_actor_with_mesh(box_);

        assert_close(a1->aabb().max_dimension(), 1.0f, 0.0001f);

        a1->move_to(791, 58, -810);

        std::vector<LightID> lights;
        std::vector<StageNode*> nodes;
        FrustumPartitioner partitioner(stage_);

        partitioner.lights_and_geometry_visible_from(
            camera, lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
    }

    void test_nodes_returned_if_never_culled() {
        auto stage = scene->new_stage();
        auto camera = stage->new_camera();
        auto a1 = stage->new_actor_with_mesh(box_);

        a1->move_to(0, 0, 100);

        std::vector<LightID> lights;
        std::vector<StageNode*> nodes;
        FrustumPartitioner partitioner(stage);

        partitioner.lights_and_geometry_visible_from(
            camera, lights, nodes
        );

        /* Not visible */
        assert_true(std::find(nodes.begin(), nodes.end(), a1) == nodes.end());

        a1->set_cullable(false);

        partitioner.lights_and_geometry_visible_from(
            camera, lights, nodes
        );

        /* Now visible */
        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
    }

    void test_destroyed_nodes_not_returned() {
        auto camera = stage_->new_camera();

        auto a1 = stage_->new_actor_with_mesh(box_);
        auto a2 = stage_->new_actor_with_mesh(box_);
        auto a3 = stage_->new_actor_with_mesh(box_);

        a1->move_to(0, 0, -5);
        a2->move_to(0, 0, -5);
        a3->move_to(0, 0, -5);

        std::vector<LightID> lights;
        std::vector<StageNode*> nodes;

        FrustumPartitioner partitioner(stage_);

        partitioner.lights_and_geometry_visible_from(
            camera, lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a2) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a3) != nodes.end());

        a2->destroy();

        lights.clear();
        nodes.clear();

        partitioner.lights_and_geometry_visible_from(
            camera, lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a2) == nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a3) != nodes.end());
    }

private:
    StagePtr stage_;
    MeshPtr box_;
};

}
