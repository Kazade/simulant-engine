#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class PipelineTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage = window->new_stage();
        camera = stage->new_camera();

        pipeline = window->render(stage, camera);
    }

    void tear_down() {
        window->destroy_stage(stage->id());
        window->destroy_pipeline(pipeline->id());
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

        smlt::PipelinePtr pipeline = window->render(stage, camera);

        assert_true(stage->is_part_of_active_pipeline());

        window->destroy_pipeline(pipeline);

        assert_false(stage->is_part_of_active_pipeline());

        pipeline = window->render(stage, camera);
        smlt::PipelinePtr pipeline2 = window->render(stage, camera);

        assert_true(stage->is_part_of_active_pipeline());

        pipeline->destroy();

        assert_true(stage->is_part_of_active_pipeline());

        pipeline2->destroy();

        assert_false(stage->is_part_of_active_pipeline());
    }

private:
    StagePtr stage;
    CameraPtr camera;
    PipelinePtr pipeline;
};

}
