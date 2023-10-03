#pragma once

#include "simulant/simulant.h"


namespace {

using namespace smlt;

class Listener : public CollisionListener {
public:
    Listener(bool* enter_called, uint32_t* stay_count, bool* leave_called):
        enter_called(enter_called),
        stay_count(stay_count),
        leave_called(leave_called) {}

    void on_collision_enter(const Collision& collision) override {
        _S_UNUSED(collision);

        if(enter_called) {
            *enter_called = true;
        }
    }

    void on_collision_stay() override {
        if(stay_count) {
            *stay_count = *stay_count + 1;
        }
    }

    void on_collision_exit(const Collision& collision) override {
        _S_UNUSED(collision);

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

        physics = scene->start_service<PhysicsService>();
        physics->set_gravity(Vec3());
        stage = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        scene->stop_service<PhysicsService>();
        SimulantTestCase::tear_down();
    }

    void test_create_kinematic_body() {
        auto body = scene->create_node<KinematicBody>();
        assert_equal(body->type(), PHYSICS_BODY_TYPE_KINEMATIC);
    }

    void test_box_collider_addition() {
        auto body = scene->create_node<RigidBody>();

        body->add_box_collider(Vec3(2, 2, 1), PhysicsMaterial::WOOD);

        auto hit = physics->ray_cast(Vec3(0, 2, 0), Vec3(0, -1, 0), 2);

        float distance = hit->distance;

        assert_true(hit);
        assert_close(distance, 1.0f, 0.0001f);

        // Check that the box doesn't extend to 3 on the X-axis
        hit = physics->ray_cast(Vec3(3, 2, 0), Vec3(0, -1, 0), 2);
        assert_false(hit);

        // But it does extend to 0.9 on the X-axis
        hit = physics->ray_cast(Vec3(0.9, 2, 0), Vec3(0, -1, 0), 2);
        assert_true(hit);

        // Check that the local offset is respected
        body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD, 0, Vec3(5, 0, 0));
        hit = physics->ray_cast(Vec3(5.0, 2, 0), Vec3(0, -1, 0), 2);
        assert_true(hit);
        assert_close(1.5f, hit->distance, 0.0001f);
    }

    void test_capsule_collider_addition() {
        auto body = scene->create_node<RigidBody>();
        body->add_capsule_collider(
            2.0f,
            2.0f,
            PhysicsMaterial::WOOD
        );

        auto hit = physics->ray_cast(Vec3(-2, 0, 0), Vec3(1, 0, 0), 2);
        assert_true(hit);
        assert_close(hit->distance, 1.0f, 0.0001f);
    }

    void test_sphere_collider_addition() {
        auto body = scene->create_node<RigidBody>();
        body->add_sphere_collider(2.0, PhysicsMaterial::WOOD);

        auto hit = physics->ray_cast(Vec3(0, 2, 0), Vec3(0, -1, 0), 2);

        assert_true(hit);
        assert_close(hit->distance, 1.0f, 0.0001f);
    }

    void test_contact_filtering() {
        bool enter_called = false;
        bool leave_called = false;

        // Create a CollisionListener
        Listener listener(&enter_called, nullptr, &leave_called);

        // Create body A
        uint16_t kind_a = 1;
        auto body = scene->create_node<StaticBody>();
        body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD, kind_a);
        body->register_collision_listener(&listener); // Register the listener

        // Create overlapping body B!
        uint16_t kind_b = 2;
        auto body2 = scene->create_node<RigidBody>();
        body2->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD, kind_b);

        class ContactFilter1 : public smlt::ContactFilter {
        public:
            bool should_collide(const smlt::Fixture *lhs, const smlt::Fixture *rhs) const override {
                return (lhs->kind() == 1 && rhs->kind() == 2) ||
                       (lhs->kind() == 2 && rhs->kind() == 1);
            }
        };

        class ContactFilter2 : public smlt::ContactFilter {
        public:
            bool should_collide(const smlt::Fixture *lhs, const smlt::Fixture *rhs) const override {
                return lhs->kind() == rhs->kind();
            }
        };

        ContactFilter1 filter1;
        ContactFilter2 filter2;

        physics->set_contact_filter(&filter1);

        // Run physics
        physics->fixed_update(1.0f / 60.0f);

        /* Should collide, because the contact filter says that the two
         * kinds do so */
        assert_true(enter_called);

        // Move away (should still not call anything)
        body2->transform->set_translation(Vec3(0, 10, 0));

        physics->fixed_update(1.0f / 60.0f);
        assert_true(leave_called);

        enter_called = false;
        physics->set_contact_filter(&filter2);

        // Move back
        body2->transform->set_translation(Vec3(0, 0, 0));
        physics->fixed_update(1.0f / 60.0f);

        /* Should *not* collide, because the contact filter says that the two
         * kinds do not */
        assert_false(enter_called);
    }

    void test_mesh_collider_addition() {
        auto mesh = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_box("mesh", scene->assets->new_material(), 1.0, 1.0, 1.0);
        auto body = scene->create_node<StaticBody>();
        body->add_mesh_collider(mesh, PhysicsMaterial::WOOD);

        auto hit = physics->ray_cast(Vec3(0, 2, 0), Vec3(0.0, -1, 0), 2);

        assert_true(hit);
        assert_close(hit->distance, 1.5f, 0.0001f);
    }

    void test_collision_listener_enter() {
        bool enter_called = false;
        bool leave_called = false;

        // Create a CollisionListener
        Listener listener(&enter_called, nullptr, &leave_called);

        // Create body A
        auto body = scene->create_node<StaticBody>();
        body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener); // Register the listener

        // Create overlapping body B!
        auto body2 = scene->create_node<RigidBody>();
        body2->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD);

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
        body2->transform->set_translation(Vec3(0, 10, 0));

        physics->fixed_update(1.0f / 60.0f);
        assert_false(enter_called);
        assert_true(leave_called);

        // Move back, should now call
        body2->transform->set_translation(Vec3(0, 0, 0));
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

        auto body = scene->create_node<StaticBody>();
        body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto body2 = scene->create_node<RigidBody>();
        body2->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD);

        physics->fixed_update(1.0f / 60.0f);

        assert_true(enter_called);
        assert_false(leave_called);

        body2->destroy();

        // Run cleanup
        application->run_frame();

        assert_true(leave_called);

        body->unregister_collision_listener(&listener);
    }

    void test_add_mesh_collider() {
        auto mesh = scene->assets->new_mesh_as_cube_with_submesh_per_face(10.0f);
        auto body = scene->create_node<StaticBody>();
        body->add_mesh_collider(mesh, PhysicsMaterial::WOOD);
    }

    void test_collision_listener_stay() {
        skip_if(true, "Not yet implemented");

        uint32_t stay_count = 0;

        Listener listener(nullptr, &stay_count, nullptr);

        auto body = scene->create_node<StaticBody>();
        body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD);
        body->register_collision_listener(&listener);

        auto actor2 = scene->create_node<smlt::Stage>();
        auto body2 = scene->create_node<RigidBody>();
        body2->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::WOOD);

        assert_false(stay_count);

        physics->fixed_update(1.0f / 60.0f);
        application->run_frame();

        assert_equal(stay_count, 1u);

        physics->fixed_update(1.0f / 60.0f);
        application->run_frame();

        assert_equal(stay_count, 2u);

        actor2->destroy();

        physics->fixed_update(1.0f / 60.0f);
        application->run_frame();

        assert_equal(stay_count, 1u);

        body->unregister_collision_listener(&listener);
    }
private:
    PhysicsService* physics;
    StagePtr stage;
};

}
