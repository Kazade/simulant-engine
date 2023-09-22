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

        stage_ = scene->create_node<smlt::Stage>();
        box_ = application->shared_assets->new_mesh_as_cube_with_submesh_per_face(1.0f);
    }

    void tear_down() override {
        test::SimulantTestCase::tear_down();

        stage_->destroy();
    }

    void test_only_renders_correct_stage() {
        auto camera = scene->create_node<smlt::Camera>();
        camera->move_to(784, 58, -775);

        auto stage2 = scene->create_node<smlt::Stage>();

        auto a1 = scene->create_node<smlt::Actor>(box_);
        a1->set_parent(stage_);

        auto a2 = scene->create_node<smlt::Actor>(box_);
        a2->set_parent(stage2);

        a1->move_to(791, 58, -810);
        a2->move_to(791, 58, -810);

        std::vector<StageNodeID> lights;
        std::vector<StageNode*> nodes;
        FrustumPartitioner partitioner(stage_);

        partitioner.lights_and_geometry_visible_from(
            camera->id(), lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a2) == nodes.end());

        stage2->destroy();
    }

    void test_visibility() {
        auto camera = scene->create_node<smlt::Camera>();
        camera->move_to(784, 58, -775);

        auto a1 = scene->create_node<smlt::Actor>(box_);

        assert_true(a1->has_any_mesh());
        assert_close(a1->aabb().max_dimension(), 1.0f, 0.0001f);

        a1->move_to(791, 58, -810);

        assert_false(a1->transformed_aabb().has_zero_area());

        std::vector<StageNodeID> lights;
        std::vector<StageNode*> nodes;
        FrustumPartitioner partitioner(stage_);

        partitioner.lights_and_geometry_visible_from(
            camera->id(), lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
    }

    void test_nodes_returned_if_never_culled() {
        auto stage = scene->create_node<smlt::Stage>();
        auto camera = scene->create_node<smlt::Camera>();
        auto a1 = scene->create_node<smlt::Actor>(box_);

        a1->move_to(0, 0, 100);

        std::vector<StageNodeID> lights;
        std::vector<StageNode*> nodes;
        FrustumPartitioner partitioner(stage);

        partitioner.lights_and_geometry_visible_from(
            camera->id(), lights, nodes
        );

        /* Not visible */
        assert_true(std::find(nodes.begin(), nodes.end(), a1) == nodes.end());

        a1->set_cullable(false);

        partitioner.lights_and_geometry_visible_from(
            camera->id(), lights, nodes
        );

        /* Now visible */
        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
    }

    void test_destroyed_nodes_not_returned() {
        auto camera = scene->create_node<smlt::Camera>();

        auto a1 = scene->create_node<smlt::Actor>(box_);
        auto a2 = scene->create_node<smlt::Actor>(box_);
        auto a3 = scene->create_node<smlt::Actor>(box_);

        a1->move_to(0, 0, -5);
        a2->move_to(0, 0, -5);
        a3->move_to(0, 0, -5);

        std::vector<StageNodeID> lights;
        std::vector<StageNode*> nodes;

        FrustumPartitioner partitioner(stage_);

        partitioner.lights_and_geometry_visible_from(
            camera->id(), lights, nodes
        );

        assert_true(std::find(nodes.begin(), nodes.end(), a1) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a2) != nodes.end());
        assert_true(std::find(nodes.begin(), nodes.end(), a3) != nodes.end());

        a2->destroy();

        lights.clear();
        nodes.clear();

        partitioner.lights_and_geometry_visible_from(
            camera->id(), lights, nodes
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
