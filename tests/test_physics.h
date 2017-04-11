#pragma once

#include "simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class ColliderTests : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        physics = controllers::RigidBodySimulation::create(window->time_keeper);
        stage = window->new_stage().fetch();
    }

    void test_box_collider_addition() {
        auto actor1 = stage->new_actor().fetch();

        auto body = actor1->new_controller<controllers::RigidBody>(physics.get());

        body->add_box_collider(Vec3(2, 2, 1), controllers::PhysicsMaterial::WOOD);

        float distance = 0;
        auto hit = physics->intersect_ray(Vec3(0, 2, 0), Vec3(0, -2, 0), &distance);

        assert_true(hit.second);
        assert_close(distance, 1.0, 0.0001);

        // Check that the box doesn't extend to 3 on the X-axis
        hit = physics->intersect_ray(Vec3(3, 2, 0), Vec3(0, -2, 0), &distance);
        assert_false(hit.second);

        // But it does extend to 0.9 on the X-axis
        hit = physics->intersect_ray(Vec3(0.9, 2, 0), Vec3(0, -2, 0), &distance);
        assert_true(hit.second);

        // Check that the local offset is respected
        body->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD, Vec3(5, 0, 0));
        hit = physics->intersect_ray(Vec3(5.0, 2, 0), Vec3(0, -2, 0), &distance);
        assert_true(hit.second);
        assert_close(1.5, distance, 0.0001);
    }

    void test_sphere_collider_addition() {
        auto actor1 = stage->new_actor().fetch();

        auto body = actor1->new_controller<controllers::RigidBody>(physics.get());
        body->add_sphere_collider(2.0, controllers::PhysicsMaterial::WOOD);

        float distance = 0;
        auto hit = physics->intersect_ray(Vec3(0, 2, 0), Vec3(0, -2, 0), &distance);

        assert_true(hit.second);
        assert_close(distance, 1.0, 0.0001);
    }

    void test_mesh_collider_addition() {
        auto mesh_id = stage->assets->new_mesh_as_box(1.0, 1.0, 1.0);
        auto actor1 = stage->new_actor().fetch();
        auto body = actor1->new_controller<controllers::StaticBody>(physics.get());
        body->add_mesh_collider(mesh_id, controllers::PhysicsMaterial::WOOD);

        float distance = 0;
        auto hit = physics->intersect_ray(Vec3(0.1, 2, 0), Vec3(0.0, -2, 0), &distance);

        assert_true(hit.second);
        assert_close(distance, 1.5, 0.0001);
    }

private:
    std::shared_ptr<controllers::RigidBodySimulation> physics;
    StagePtr stage;
};

}
