#pragma once

#include <simulant/test.h>
#include "../simulant/behaviours/movement/cylindrical_billboard.h"

namespace {

using namespace smlt;

class CylindricalBillboardTests : public test::SimulantTestCase {
public:

    void test_basic_usage() {
        auto stage = window->new_stage();
        auto actor = stage->new_actor();
        auto camera = stage->new_camera();

        auto pipeline = window->render(stage, camera).as_pipeline();
        pipeline->activate();

        actor->new_behaviour<behaviours::CylindricalBillboard>(camera);

        camera->move_to(0, 0, 100);

        window->run_frame();
        assert_equal(actor->forward(), Vec3(0, 0, 1));

        camera->move_to(0, 100, 0);

        window->run_frame();

        // Default to negative Z
        assert_equal(actor->forward(), Vec3(0, 0, -1));

        camera->move_to(100, 0, 0);

        window->run_frame();
        assert_equal(actor->forward(), Vec3(1, 0, 0));
    }
};

}
