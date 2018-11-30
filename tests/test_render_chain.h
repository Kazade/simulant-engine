#ifndef TEST_RENDER_CHAIN_H
#define TEST_RENDER_CHAIN_H

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class RenderChainTests : public smlt::test::SimulantTestCase {
public:
    void test_basic_usage() {
        Viewport view;
        auto stage = window->new_stage();
        auto cam = stage->new_camera();
        TextureID tex = window->shared_assets->new_texture();

        PipelinePtr pipeline1 = window->render(stage, cam);
        PipelinePtr pipeline2 = window->render(stage, cam).to_texture(tex);
        PipelinePtr pipeline3 = window->render(stage, cam).to_framebuffer(view);
        PipelinePtr pipeline4 = window->render(stage, cam).with_priority(RENDER_PRIORITY_FOREGROUND);

        assert_equal(cam->id(), pipeline1->camera_id());
        assert_equal(stage->id(), pipeline1->stage_id());
        assert_equal(TextureID(), pipeline1->target_id());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline1->priority());

        assert_equal(cam->id(), pipeline2->camera_id());
        assert_equal(stage->id(), pipeline2->stage_id());
        assert_equal(tex, pipeline2->target_id());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline2->priority());

        assert_equal(cam->id(), pipeline3->camera_id());
        assert_equal(stage->id(), pipeline3->stage_id());
        assert_equal(TextureID(), pipeline3->target_id());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline3->priority());

        assert_equal(cam->id(), pipeline4->camera_id());
        assert_equal(stage->id(), pipeline4->stage_id());
        assert_equal(TextureID(), pipeline4->target_id());
        assert_equal(RENDER_PRIORITY_FOREGROUND, pipeline4->priority());

        pipeline1->deactivate();

        assert_false(window->is_pipeline_enabled(pipeline1->id()));

        auto pid2 = pipeline2->id();
        window->delete_pipeline(pid2);
        assert_false(window->has_pipeline(pid2));

        pipeline1->activate();
        assert_true(window->is_pipeline_enabled(pipeline1->id()));
    }

    void test_rendering_flag() {
        auto stage = window->new_stage();
        auto cam = stage->new_camera();

        assert_false(stage->is_being_rendered());

        PipelinePtr pipeline = window->render(stage, cam);

        assert_true(stage->is_being_rendered());

        pipeline->deactivate();

        assert_false(stage->is_being_rendered());

        PipelinePtr pipeline2 = window->render(stage, cam);

        assert_true(stage->is_being_rendered());

        window->delete_pipeline(pipeline2->id());

        assert_false(stage->is_being_rendered());

        pipeline->activate();

        assert_true(stage->is_being_rendered());

        window->delete_pipeline(pipeline->id());

        assert_false(stage->is_being_rendered());
    }
};


}

#endif // TEST_RENDER_CHAIN_H
