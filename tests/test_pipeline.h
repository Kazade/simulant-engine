#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "global.h"

namespace {

using namespace smlt;

class PipelineTests : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage = window->new_stage();
        camera = stage->new_camera();

        pipeline = window->render(stage, camera);
    }

    void tear_down() {
        window->delete_stage(stage->id());
        window->delete_pipeline(pipeline->id());
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

private:
    StagePtr stage;
    CameraPtr camera;
    PipelinePtr pipeline;
};

}
