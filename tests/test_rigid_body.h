#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


class RigidBodyTest : public smlt::test::SimulantTestCase {
public:
    void test_adding_to_stage_node_inherits_location() {
        auto stage = scene->create_node<smlt::Stage>();
        auto actor = scene->create_node<smlt::Stage>();

        actor->move_to(10, 0, 0);
        actor->rotate_x_by(smlt::Degrees(90));

        scene->start_service<PhysicsService>();
        auto controller = scene->create_node<RigidBody>();

        assert_equal(controller->position().x, 10.0f);
        assert_equal(controller->position().y, 0.0f);
        assert_equal(controller->position().z, 0.0f);

        stage->destroy();
    }

    void test_set_mass() {
        auto controller = scene->create_node<smlt::RigidBody>();

        assert_equal(controller->mass(), 1.0f);

        controller->add_box_collider(smlt::Vec3(10.0f), smlt::PhysicsMaterial::IRON);
        assert_true(controller->mass() > 1.0f);

        controller->set_mass(100.0f);
        assert_equal(controller->mass(), 100.0f);

        controller->set_mass(50.0f);
        assert_equal(controller->mass(), 50.0f);
    }
};
