#ifndef TEST_OBJECT_H
#define TEST_OBJECT_H

#include "kglt/kglt.h"
#include "kglt/kazbase/testing.h"

#include "global.h"

class ObjectTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_set_absolute_position() {
        kglt::ActorID act = window->scene().stage().new_actor();
        kglt::Actor* actor = &window->scene().stage().actor(act);

        actor->set_absolute_position(10, 10, 10);

        assert_equal(kglt::Vec3(10, 10, 10), actor->absolute_position());

        kglt::ActorID act2 = window->scene().stage().new_actor();
        kglt::Actor* actor2 = &window->scene().stage().actor(act2);

        actor2->set_parent(actor);

        //Should be the same as its parent
        assert_equal(actor2->absolute_position(), actor->absolute_position());

        //Make sure relative position is correctly calculated
        actor2->set_absolute_position(20, 10, 10);
        assert_equal(kglt::Vec3(10, 0, 0), actor2->relative_position());
        assert_equal(kglt::Vec3(20, 10, 10), actor2->absolute_position());

        //Make sure setting by vec3 works
        actor2->set_absolute_position(kglt::Vec3(0, 0, 0));
        assert_equal(kglt::Vec3(), actor2->absolute_position());
    }

    void test_set_relative_position() {
        kglt::ActorID act = window->scene().stage().new_actor();
        kglt::Actor* actor = &window->scene().stage().actor(act);

        actor->set_relative_position(10, 10, 10);

        //No parent, both should be the same
        assert_equal(kglt::Vec3(10, 10, 10), actor->relative_position());
        assert_equal(kglt::Vec3(10, 10, 10), actor->absolute_position());

        kglt::ActorID act2 = window->scene().stage().new_actor();
        kglt::Actor* actor2 = &window->scene().stage().actor(act2);

        actor2->set_parent(actor);

        actor2->set_relative_position(kglt::Vec3(10, 0, 0));

        assert_equal(kglt::Vec3(20, 10, 10), actor2->absolute_position());
        assert_equal(kglt::Vec3(10, 0, 0), actor2->relative_position());
    }
};

#endif // TEST_OBJECT_H
