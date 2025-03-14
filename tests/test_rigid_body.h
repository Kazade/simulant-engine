#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class DynamicBodyTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        smlt::test::SimulantTestCase::set_up();
        scene->start_service<PhysicsService>();
    }

    void tear_down() {
        scene->stop_service<PhysicsService>();
        smlt::test::SimulantTestCase::tear_down();
    }

    void test_adding_to_stage_node_inherits_location() {
        auto stage = scene->create_child<smlt::Stage>();
        auto actor = scene->create_child<smlt::Stage>();

        actor->transform->set_translation(Vec3(10, 0, 0));
        actor->transform->rotate(smlt::Vec3::right(), smlt::Degrees(90));

        scene->start_service<PhysicsService>();
        auto controller = scene->create_child<DynamicBody>();
        controller->set_parent(actor);

        assert_equal(controller->transform->position().x, 10.0f);
        assert_equal(controller->transform->position().y, 0.0f);
        assert_equal(controller->transform->position().z, 0.0f);

        stage->destroy();
    }

    void test_set_mass() {
        auto controller = scene->create_child<smlt::DynamicBody>();
        assert_equal(controller->mass(), 1.0f);

        controller->add_box_collider(smlt::Vec3(10.0f),
                                     smlt::PhysicsMaterial::iron());
        assert_true(controller->mass() > 1.0f);

        controller->set_mass(100.0f);
        assert_equal(controller->mass(), 100.0f);

        controller->set_mass(50.0f);
        assert_equal(controller->mass(), 50.0f);
    }
};

}
