#ifndef TEST_RESPONSIVE_H
#define TEST_RESPONSIVE_H

#include "kglt/kglt.h"
#include "kglt/kazbase/testing.h"
#include "global.h"

class ResponsiveTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        window->scene().enable_physics(DefaultPhysicsEngine::create());
        //window->reset();
    }

    void test_set_absolute_rotation() {
        kglt::ActorID act = window->scene().stage().new_actor(true);
        kglt::Actor* actor = &window->scene().stage().actor(act);

        actor->set_absolute_rotation(kglt::Degrees(10), 0, 0, 1);

        assert_equal(actor->relative_rotation(), actor->absolute_rotation());
        assert_equal(actor->absolute_rotation(), actor->responsive_body().rotation());

        kglt::ActorID act2 = window->scene().stage().new_actor(true);
        kglt::Actor* actor2 = &window->scene().stage().actor(act2);

        actor2->set_parent(actor);

        assert_equal(actor2->absolute_rotation(), actor->absolute_rotation());

        actor2->set_absolute_rotation(kglt::Degrees(20), 0, 0, 1);

        kglt::Quaternion expected_rel, expected_abs;
        kmQuaternionRotationAxisAngle(&expected_abs, &KM_VEC3_POS_Z, kmDegreesToRadians(20));
        kmQuaternionRotationAxisAngle(&expected_rel, &KM_VEC3_POS_Z, kmDegreesToRadians(10));

        assert_equal(expected_abs, actor2->absolute_rotation());
        assert_equal(expected_rel, actor2->relative_rotation());
    }

    void test_set_absolute_position() {
        kglt::ActorID act = window->scene().stage().new_actor(true);
        kglt::Actor* actor = &window->scene().stage().actor(act);

        actor->set_absolute_position(10, 10, 10);

        assert_equal(kglt::Vec3(10, 10, 10), actor->absolute_position());
        assert_equal(actor->absolute_position(), actor->responsive_body().position());

        kglt::ActorID act2 = window->scene().stage().new_actor(true);
        kglt::Actor* actor2 = &window->scene().stage().actor(act2);

        actor2->set_parent(actor);

        //Should be the same as its parent
        assert_equal(actor2->absolute_position(), actor->absolute_position());
        assert_equal(actor2->absolute_position(), actor2->responsive_body().position());

        //Make sure relative position is correctly calculated
        actor2->set_absolute_position(20, 10, 10);
        assert_equal(kglt::Vec3(10, 0, 0), actor2->relative_position());
        assert_equal(kglt::Vec3(20, 10, 10), actor2->absolute_position());

        //Make sure setting by vec3 works
        actor2->set_absolute_position(kglt::Vec3(0, 0, 0));
        assert_equal(kglt::Vec3(), actor2->absolute_position());
    }

    void test_set_relative_position() {
        kglt::ActorID act = window->scene().stage().new_actor(true);
        kglt::Actor* actor = &window->scene().stage().actor(act);

        actor->set_relative_position(10, 10, 10);

        //No parent, both should be the same
        assert_equal(kglt::Vec3(10, 10, 10), actor->relative_position());
        assert_equal(kglt::Vec3(10, 10, 10), actor->absolute_position());

        kglt::ActorID act2 = window->scene().stage().new_actor(true);
        kglt::Actor* actor2 = &window->scene().stage().actor(act2);

        actor2->set_parent(actor);

        actor2->set_relative_position(kglt::Vec3(10, 0, 0));

        assert_equal(kglt::Vec3(20, 10, 10), actor2->absolute_position());
        assert_equal(kglt::Vec3(10, 0, 0), actor2->relative_position());
    }
};

#endif // TEST_RESPONSIVE_H
