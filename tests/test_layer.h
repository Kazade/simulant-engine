#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class PipelineTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage = scene->create_child<smlt::Stage>();
        camera = scene->create_child<smlt::Camera>();

        pipeline = window->compositor->create_layer(stage, camera);
    }

    void tear_down() {
        stage->destroy();
        pipeline->destroy();
    }

    void test_find_layer_with_name() {
        auto p1 = window->compositor->create_layer(stage, camera)->set_name("pipeline1");
        auto p2 = window->compositor->create_layer(stage, camera)->set_name("pipeline2");

        auto found = window->compositor->find_layer("pipeline1");
        assert_equal(p1->name(), found->name());

        found = window->compositor->find_layer("pipeline2");
        assert_equal(p2->name(), found->name());

        found = window->compositor->find_layer("bananas");
        assert_false(found);
    }

    void test_detail_range_settings() {

        pipeline->set_detail_level_distances(
            10.0f, 20.0f, 30.0f, 40.0f
        );

        assert_equal(pipeline->detail_level_at_distance(1.0), DETAIL_LEVEL_NEAREST);
        assert_equal(pipeline->detail_level_at_distance(10.0), DETAIL_LEVEL_NEAR);
        assert_equal(pipeline->detail_level_at_distance(25.0), DETAIL_LEVEL_MID);
        assert_equal(pipeline->detail_level_at_distance(35.0), DETAIL_LEVEL_FAR);
        assert_equal(pipeline->detail_level_at_distance(50.0), DETAIL_LEVEL_FARTHEST);
    }

    void test_stage_active_counting() {
        /* When a stage is attached to an pipeline a counter should
         * increment and drive is_part_of_active_pipeline */
        pipeline->destroy();

        assert_false(stage->is_part_of_active_pipeline());

        smlt::LayerPtr pipeline = window->compositor->create_layer(stage, camera);
        pipeline->activate();

        assert_true(stage->is_part_of_active_pipeline());

        pipeline->destroy();

        assert_false(stage->is_part_of_active_pipeline());

        pipeline = window->compositor->create_layer(stage, camera);
        pipeline->activate();

        smlt::LayerPtr pipeline2 = window->compositor->create_layer(stage, camera);
        pipeline2->activate();

        assert_true(stage->is_part_of_active_pipeline());

        pipeline->destroy();

        assert_true(stage->is_part_of_active_pipeline());

        pipeline2->destroy();

        assert_false(stage->is_part_of_active_pipeline());
    }

    void test_pipeline_starts_deactivated() {
        auto p = window->compositor->create_layer(stage, camera);
        assert_false(p->is_active());
    }

    void test_changing_camera() {
        auto p = window->compositor->create_layer(stage, camera);
        p = p->set_camera(scene->create_child<smlt::Camera>());

        assert_not_equal(p->camera(), camera);
    }

private:
    StagePtr stage;
    CameraPtr camera;
    LayerPtr pipeline;
};

}
