#pragma once

#include "simulant/simulant.h"


namespace {

using namespace smlt;

class Listener : public behaviours::CollisionListener {
public:
    Listener(bool* enter_called, uint32_t* stay_count, bool* leave_called):
        enter_called(enter_called),
        stay_count(stay_count),
        leave_called(leave_called) {}

    void on_collision_enter(const behaviours::Collision& collision) override {
        if(enter_called) {
            *enter_called = true;
        }
    }

    void on_collision_stay() override {
        if(stay_count) {
            *stay_count = *stay_count + 1;
        }
    }

    void on_collision_exit(const behaviours::Collision& collision) override {
        if(leave_called) {
            *leave_called = true;
        }
    }

    bool* enter_called = nullptr;
    uint32_t* stay_count = nullptr;
    bool* leave_called = nullptr;
};

class ColliderTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        physics = behaviours::RigidBodySimulation::create(window->time_keeper);
        physics->set_gravity(Vec3());
        stage = window->new_stage();
    }

    void tear_down() {
        window->destroy_all_stages();
        physics.reset();
        SimulantTestCase::tear_down();
    }

    void test_box_collider_addition() {
        auto actor1 = stage->new_actor();

        auto body = actor1->new_behaviour<behaviours::RigidBody>(physics.get());

        body->add_box_collider(Vec3(2, 2, 1), behaviours::PhysicsMaterial::WOOD);

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
        body->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD, Vec3(5, 0, 0));
        hit = physics->intersect_ray(Vec3(5.0, 2, 0), Vec3(0, -2, 0), &distance);
        assert_true(hit.second);
        assert_close(1.5, distance, 0.0001);
    }

    void test_sphere_collider_addition() {
        auto actor1 = stage->new_actor();

        auto body = actor1->new_behaviour<behaviours::RigidBody>(physics.get());
        body->add_sphere_collider(2.0, behaviours::PhysicsMaterial::WOOD);

        float distance = 0;
        auto hit = physics->intersect_ray(Vec3(0, 2, 0), Vec3(0, -2, 0), &distance);

        assert_true(hit.second);
        assert_close(distance, 1.0, 0.0001);
    }

    void test_mesh_collider_addition() {
        auto mesh = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_box("mesh", stage->assets->new_material(), 1.0, 1.0, 1.0);
        auto actor1 = stage->new_actor();
        auto body = actor1->new_behaviour<behaviours::StaticBody>(physics.get());
        body->add_mesh_collider(mesh, behaviours::PhysicsMaterial::WOOD);

        float distance = 0;
        auto hit = physics->intersect_ray(Vec3(0, 2, 0), Vec3(0.0, -2, 0), &distance);

        assert_true(hit.second);
        assert_close(distance, 1.5, 0.0001);
    }

    void test_collision_listener_enter() {
        bool enter_called = false;
        bool leave_called = false;

        // Create a CollisionListener
        Listener listener(&enter_called, nullptr, &leave_called);

        // Create body A
        auto actor1 = stage->new_actor();
        auto body = actor1->new_behaviour<behaviours::StaticBody>(physics.get());
        body->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener); // Register the listener

        // Create overlapping body B!
        auto actor2 = stage->new_actor();
        auto body2 = actor2->new_behaviour<behaviours::RigidBody>(physics.get());
        body2->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD);

        // Run physics
        physics->fixed_update(1.0f / 60.0f);
        assert_true(enter_called);
        assert_false(leave_called);

        enter_called = false; // Reset

        // Shouldn't call again!
        physics->fixed_update(1.0f / 60.0f);
        assert_false(enter_called);
        assert_false(leave_called);

        // Move away (should still not call anything)
        body2->move_to(Vec3(0, 10, 0));

        physics->fixed_update(1.0f / 60.0f);
        assert_false(enter_called);
        assert_true(leave_called);        

        // Move back, should now call        
        body2->move_to(Vec3(0, 0, 0));
        body2->set_linear_velocity(Vec3(0, 0, 0));

        /* The first step new contacts will be created, but new signals won't fire until the next step */
        physics->fixed_update(1.0f / 60.0f);
        physics->fixed_update(1.0f / 60.0f);
        assert_true(enter_called);

        body->unregister_collision_listener(&listener);
    }

    void test_collision_listener_leave() {
        bool enter_called = false;
        bool leave_called = false;

        Listener listener(&enter_called, nullptr, &leave_called);

        auto actor1 = stage->new_actor();
        auto body = actor1->new_behaviour<behaviours::StaticBody>(physics.get());
        body->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto actor2 = stage->new_actor();
        auto body2 = actor2->new_behaviour<behaviours::RigidBody>(physics.get());
        body2->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD);

        physics->fixed_update(1.0 / 60.0f);

        assert_true(enter_called);
        assert_false(leave_called);

        actor2->destroy();

        // Run cleanup
        window->run_frame();

        assert_true(leave_called);

        body->unregister_collision_listener(&listener);
    }

    void test_collision_listener_stay() {
        skip_if(true, "Not yet implemented");

        uint32_t stay_count = 0;

        Listener listener(nullptr, &stay_count, nullptr);

        auto actor1 = stage->new_actor();
        auto body = actor1->new_behaviour<behaviours::StaticBody>(physics.get());
        body->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto actor2 = stage->new_actor();
        auto body2 = actor2->new_behaviour<behaviours::RigidBody>(physics.get());
        body2->add_box_collider(Vec3(1, 1, 1), behaviours::PhysicsMaterial::WOOD);

        assert_false(stay_count);

        physics->fixed_update(1.0f / 60.0f);
        window->run_frame();

        assert_equal(stay_count, 1u);

        physics->fixed_update(1.0f / 60.0f);
        window->run_frame();

        assert_equal(stay_count, 2u);

        actor2->destroy();

        physics->fixed_update(1.0f / 60.0f);
        window->run_frame();

        assert_equal(stay_count, 1u);

        body->unregister_collision_listener(&listener);
    }
private:
    std::shared_ptr<behaviours::RigidBodySimulation> physics;
    StagePtr stage;
};

}
