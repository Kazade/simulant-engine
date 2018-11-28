#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


class RigidBodyTest : public smlt::test::SimulantTestCase {
public:
    void test_adding_to_stage_node_inherits_location() {
        smlt::StagePtr stage = window->new_stage();
        smlt::ActorPtr actor = stage->new_actor();

        actor->move_to(10, 0, 0);
        actor->rotate_x_by(smlt::Degrees(90));

        auto simulation = smlt::behaviours::RigidBodySimulation::create(window->time_keeper);
        auto controller = actor->new_behaviour<smlt::behaviours::RigidBody>(simulation.get());

        assert_equal(controller->position().x, 10.0f);
        assert_equal(controller->position().y, 0.0f);
        assert_equal(controller->position().z, 0.0f);

        window->delete_stage(stage->id());
    }
};
