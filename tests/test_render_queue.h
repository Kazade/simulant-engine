#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class RenderQueueTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->new_stage();
    }

    void tear_down() {
        window->destroy_stage(stage_->id());
    }

    void test_render_group_key_generation() {
        batcher::RenderGroup pass0_unblended_100_tex1 = {batcher::generate_render_group_key(
            0, false, 100.0f
        )};

        batcher::RenderGroup  pass0_unblended_10_tex1 = {batcher::generate_render_group_key(
            0, false, 10.0f
        )};

        batcher::RenderGroup  pass0_blended_100_tex1 = {batcher::generate_render_group_key(
            0, true, 100.0f
        )};

        batcher::RenderGroup  pass0_blended_10_tex1 = {batcher::generate_render_group_key(
            0, true, 10.0f
        )};

        batcher::RenderGroup  pass1_blended_10_tex1 = {batcher::generate_render_group_key(
            1, true, 10.0f
        )};

        assert_true(pass0_unblended_100_tex1 < pass0_blended_100_tex1);
        assert_true(pass0_unblended_100_tex1 < pass0_blended_10_tex1);

        // Unblended are rendered front-to-back
        assert_true(pass0_unblended_10_tex1 < pass0_unblended_100_tex1);

        // Blended are renderered back to front
        assert_true(pass0_blended_100_tex1 < pass0_blended_10_tex1);

        // Pass always takes precedence
        assert_true(pass0_blended_10_tex1 < pass1_blended_10_tex1);
        assert_true(pass0_blended_100_tex1 < pass1_blended_10_tex1);
    }

private:
    StagePtr stage_;

};

}
