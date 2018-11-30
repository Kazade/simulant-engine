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
        window->delete_stage(stage_->id());
    }

#ifdef SIMULANT_GL_VERSION_2X
    void test_shader_grouping() {

    }
#else
    void test_shader_grouping() {}
#endif

private:
    StagePtr stage_;

};

}
