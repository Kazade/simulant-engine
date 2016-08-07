#ifndef TEST_RENDER_CHAIN_H
#define TEST_RENDER_CHAIN_H

#include "kaztest/kaztest.h"

#include "kglt/kglt.h"
#include "global.h"

namespace {

using namespace kglt;

class RenderChainTests : public KGLTTestCase {
public:
    void test_basic_usage() {
        Viewport view;
        StageID stage = window->new_stage();
        CameraID cam = window->new_camera();
        TextureID tex = window->shared_assets->new_texture();

        PipelineID pid1 = window->render(stage, cam);
        PipelineID pid2 = window->render(stage, cam).to_texture(tex);
        PipelineID pid3 = window->render(stage, cam).to_framebuffer(view);
        PipelineID pid4 = window->render(stage, cam).with_priority(kglt::RENDER_PRIORITY_FOREGROUND);

        auto pipeline1 = window->pipeline(pid1);
        assert_equal(cam, pipeline1->camera_id());
        assert_equal(stage, pipeline1->stage_id());
        assert_equal(TextureID(), pipeline1->target_id());
        assert_equal(kglt::RENDER_PRIORITY_MAIN, pipeline1->priority());

        auto pipeline2 = window->pipeline(pid2);
        assert_equal(cam, pipeline2->camera_id());
        assert_equal(stage, pipeline2->stage_id());
        assert_equal(tex, pipeline2->target_id());
        assert_equal(kglt::RENDER_PRIORITY_MAIN, pipeline2->priority());

        auto pipeline3 = window->pipeline(pid3);
        assert_equal(cam, pipeline3->camera_id());
        assert_equal(stage, pipeline3->stage_id());
        assert_equal(TextureID(), pipeline3->target_id());
        assert_equal(kglt::RENDER_PRIORITY_MAIN, pipeline3->priority());

        auto pipeline4 = window->pipeline(pid4);
        assert_equal(cam, pipeline4->camera_id());
        assert_equal(stage, pipeline4->stage_id());
        assert_equal(TextureID(), pipeline4->target_id());
        assert_equal(kglt::RENDER_PRIORITY_FOREGROUND, pipeline4->priority());

        window->disable_pipeline(pid1);

        assert_false(window->is_pipeline_enabled(pid1));

        window->delete_pipeline(pid2);
        assert_false(window->has_pipeline(pid2));

        window->enable_pipeline(pid1);
        assert_true(window->is_pipeline_enabled(pid1));
    }

    void test_rendering_flag() {
        StageID stage = window->new_stage();
        CameraID cam = window->new_camera();

        assert_false(window->stage(stage)->is_being_rendered());

        PipelineID pid1 = window->render(stage, cam);

        assert_true(window->stage(stage)->is_being_rendered());

        window->disable_pipeline(pid1);

        assert_false(window->stage(stage)->is_being_rendered());

        PipelineID pid2 = window->render(stage, cam);

        assert_true(window->stage(stage)->is_being_rendered());

        window->delete_pipeline(pid2);

        assert_false(window->stage(stage)->is_being_rendered());

        window->enable_pipeline(pid1);

        assert_true(window->stage(stage)->is_being_rendered());

        window->delete_pipeline(pid1);

        assert_false(window->stage(stage)->is_being_rendered());
    }
};


}

#endif // TEST_RENDER_CHAIN_H
