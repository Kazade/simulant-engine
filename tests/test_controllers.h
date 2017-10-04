#pragma once

#include "global.h"

namespace {

using namespace smlt;


class TestController : public Controller, public Managed<TestController> {
public:
    TestController(Controllable* c) {}

    void on_controller_first_update(Controllable* controllable) {
        call_count++;
    }

    uint32_t call_count = 0;

    const std::string name() const { return "test controller"; }
};


class ControllerTests : public SimulantTestCase {
public:
    void test_controller_first_update() {

        auto stage = window->new_stage().fetch();
        auto actor = stage->new_actor().fetch();

        auto controller = actor->new_controller<TestController>();

        assert_equal(0, controller->call_count);

        actor->_update_thunk(0.0f);
        assert_equal(1, controller->call_count);

        // No effect a second time
        actor->_update_thunk(0.0f);
        assert_equal(1, controller->call_count);

        // Disable then re-enable
        controller->disable();
        controller->enable();

        // Called once more
        actor->_update_thunk(0.0f);
        assert_equal(2, controller->call_count);
    }
};


}
