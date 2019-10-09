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
        TextureID tex = window->shared_assets->new_texture(256, 256);

        PipelinePtr pipeline1 = window->render(stage, cam);
        PipelinePtr pipeline2 = window->render(stage, cam).to_texture(tex);
        PipelinePtr pipeline3 = window->render(stage, cam).to_framebuffer(view);
        PipelinePtr pipeline4 = window->render(stage, cam).with_priority(RENDER_PRIORITY_FOREGROUND);

        assert_equal(cam->id(), pipeline1->camera()->id());
        assert_equal(stage->id(), pipeline1->stage()->id());
        assert_equal(TexturePtr(), pipeline1->target());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline1->priority());

        assert_equal(cam->id(), pipeline2->camera()->id());
        assert_equal(stage->id(), pipeline2->stage()->id());
        assert_equal(tex, pipeline2->target()->id());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline2->priority());

        assert_equal(cam->id(), pipeline3->camera()->id());
        assert_equal(stage->id(), pipeline3->stage()->id());
        assert_equal(TexturePtr(), pipeline1->target());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline3->priority());

        assert_equal(cam->id(), pipeline4->camera()->id());
        assert_equal(stage->id(), pipeline4->stage()->id());
        assert_equal(TexturePtr(), pipeline1->target());
        assert_equal(RENDER_PRIORITY_FOREGROUND, pipeline4->priority());

        pipeline1->deactivate();

        assert_false(window->is_pipeline_enabled(pipeline1->id()));

        auto pid2 = pipeline2->id();
        window->destroy_pipeline(pid2);
        window->run_frame();

        assert_false(window->has_pipeline(pid2));

        pipeline1->activate();
        assert_true(window->is_pipeline_enabled(pipeline1->id()));
    }
};


}

#endif // TEST_RENDER_CHAIN_H
