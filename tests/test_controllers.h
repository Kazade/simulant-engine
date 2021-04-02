#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/macros.h"

namespace {

using namespace smlt;


class TestBehaviour : public Behaviour, public RefCounted<TestBehaviour> {
public:
    TestBehaviour() {}

    void on_behaviour_first_update(Organism* controllable) {
        _S_UNUSED(controllable);
        call_count++;
    }

    uint32_t call_count = 0;

    const char* name() const { return "test behaviour"; }
};


class BehaviourTests : public smlt::test::SimulantTestCase {
public:
    void test_behaviour_first_update() {

        auto stage = window->new_stage();
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
