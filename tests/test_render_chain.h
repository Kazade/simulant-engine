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
        auto stage = scene->create_child<smlt::Stage>();
        auto cam = scene->create_child<smlt::Camera>();
        auto tex = application->shared_assets->create_texture(256, 256);

        auto pipeline1 = window->compositor->create_layer(stage, cam);
        auto pipeline2 = window->compositor->create_layer(stage, cam)->set_target(tex);
        auto pipeline3 = window->compositor->create_layer(stage, cam)->set_viewport(view);
        auto pipeline4 = window->compositor->create_layer(stage, cam)->set_priority(RENDER_PRIORITY_FOREGROUND);

        pipeline2->set_name("Layer 2");

        assert_equal(cam->id(), pipeline1->camera()->id());
        assert_equal(stage->id(), pipeline1->stage_node()->id());
        assert_equal(TexturePtr(), pipeline1->target());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline1->priority());

        assert_equal(cam->id(), pipeline2->camera()->id());
        assert_equal(stage->id(), pipeline2->stage_node()->id());
        assert_equal(tex->id(), pipeline2->target()->id());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline2->priority());

        assert_equal(cam->id(), pipeline3->camera()->id());
        assert_equal(stage->id(), pipeline3->stage_node()->id());
        assert_equal(TexturePtr(), pipeline1->target());
        assert_equal(RENDER_PRIORITY_MAIN, pipeline3->priority());

        assert_equal(cam->id(), pipeline4->camera()->id());
        assert_equal(stage->id(), pipeline4->stage_node()->id());
        assert_equal(TexturePtr(), pipeline1->target());
        assert_equal(RENDER_PRIORITY_FOREGROUND, pipeline4->priority());

        pipeline1->deactivate();

        assert_false(pipeline1->is_active());

        auto pid2 = pipeline2->name();
        pipeline2->destroy();
        application->run_frame();

        assert_false(window->compositor->has_layer(pid2));

        pipeline1->activate();
        assert_true(pipeline1->is_active());
    }
};


}

#endif // TEST_RENDER_CHAIN_H
