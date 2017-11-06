#pragma once

#include "global.h"

namespace {

using namespace smlt;


class TestBehaviour : public Behaviour, public Managed<TestBehaviour> {
public:
    TestBehaviour(Organism* c) {}

    void on_behaviour_first_update(Organism* controllable) {
        call_count++;
    }

    uint32_t call_count = 0;

    const std::string name() const { return "test behaviour"; }
};


class BehaviourTests : public SimulantTestCase {
public:
    void test_behaviour_first_update() {

        auto stage = window->new_stage().fetch();
        auto actor = stage->new_actor();

        auto behaviour = actor->new_behaviour<TestBehaviour>();

        assert_equal(0u, behaviour->call_count);

        actor->_update_thunk(0.0f);
        assert_equal(1u, behaviour->call_count);

        // No effect a second time
        actor->_update_thunk(0.0f);
        assert_equal(1u, behaviour->call_count);

        // Disable then re-enable
        behaviour->disable();
        behaviour->enable();

        // Called once more
        actor->_update_thunk(0.0f);
        assert_equal(2u, behaviour->call_count);
    }
};


}
