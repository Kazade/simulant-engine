#pragma once

#include "simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class Listener : public controllers::CollisionListener {
public:
    Listener(bool* enter_called, uint32_t* stay_count, bool* leave_called):
        enter_called(enter_called) {}

    void on_collision_enter() {
        if(enter_called) {
            *enter_called = true;
        }
    }

    void on_collision_stay() {
        if(stay_count) {
            *stay_count = *stay_count + 1;
        }
    }

    void on_collision_leave() {
        if(leave_called) {
            *leave_called = true;
        }
    }

    bool* enter_called = nullptr;
    uint32_t* stay_count = nullptr;
    bool* leave_called = nullptr;
};

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

    void test_collision_listener_enter() {
        skip_if(true, "Not yet implemented");

        bool enter_called = false;

        Listener listener(&enter_called, nullptr, nullptr);

        auto actor1 = stage->new_actor().fetch();
        auto body = actor1->new_controller<controllers::StaticBody>(physics.get());
        body->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto actor2 = stage->new_actor().fetch();
        auto body2 = actor2->new_controller<controllers::RigidBody>(physics.get());
        body2->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD);

        assert_true(enter_called);
    }

    void test_collision_listener_leave() {
        skip_if(true, "Not yet implemented");

        bool leave_called = false;

        Listener listener(nullptr, nullptr, &leave_called);

        auto actor1 = stage->new_actor().fetch();
        auto body = actor1->new_controller<controllers::StaticBody>(physics.get());
        body->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto actor2 = stage->new_actor().fetch();
        auto body2 = actor2->new_controller<controllers::RigidBody>(physics.get());
        body2->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD);

        assert_false(leave_called);

        actor2->ask_owner_for_destruction();

        assert_true(leave_called);
    }

    void test_collision_listener_stay() {
        skip_if(true, "Not yet implemented");

        uint32_t stay_count = 0;

        Listener listener(nullptr, &stay_count, nullptr);

        auto actor1 = stage->new_actor().fetch();
        auto body = actor1->new_controller<controllers::StaticBody>(physics.get());
        body->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto actor2 = stage->new_actor().fetch();
        auto body2 = actor2->new_controller<controllers::RigidBody>(physics.get());
        body2->add_box_collider(Vec3(1, 1, 1), controllers::PhysicsMaterial::WOOD);

        assert_false(stay_count);

        window->run_frame();

        assert_equal(stay_count, 1);

        actor2->ask_owner_for_destruction();
        window->run_frame();

        assert_equal(stay_count, 1);
    }
private:
    std::shared_ptr<controllers::RigidBodySimulation> physics;
    StagePtr stage;
};

}
