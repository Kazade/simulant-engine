#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


class RigidBodyTest : public smlt::test::SimulantTestCase {
public:
    void test_adding_to_stage_node_inherits_location() {
        smlt::StagePtr stage = scene->new_stage();
        smlt::ActorPtr actor = stage->new_actor();

        actor->move_to(10, 0, 0);
        actor->rotate_x_by(smlt::Degrees(90));

        auto simulation = smlt::behaviours::RigidBodySimulation::create(application->time_keeper);
        auto controller = actor->new_behaviour<smlt::behaviours::RigidBody>(simulation.get());

        assert_equal(controller->position().x, 10.0f);
        assert_equal(controller->position().y, 0.0f);
        assert_equal(controller->position().z, 0.0f);

        scene->destroy_stage(stage->id());
    }

    void test_set_mass() {
        smlt::StagePtr stage = window->new_stage();
        smlt::ActorPtr actor = stage->new_actor();

        auto simulation = smlt::behaviours::RigidBodySimulation::create(window->time_keeper);
        auto controller = actor->new_behaviour<smlt::behaviours::RigidBody>(simulation.get());

        assert_equal(controller->mass(), 1.0f);

        controller->add_box_collider(smlt::Vec3(10.0f), smlt::behaviours::PhysicsMaterial::IRON);
        assert_true(controller->mass() > 1.0f);

        controller->set_mass(100.0f);
        assert_equal(controller->mass(), 100.0f);

        controller->set_mass(50.0f);
        assert_equal(controller->mass(), 50.0f);
    }
};
