#ifndef TEST_CAMERA_H
#define TEST_CAMERA_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

class CameraTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_project_point() {
        window->scene().camera().set_perspective_projection(45.0, float(window->width()) / float(window->height()));

        kmVec3 p1 = window->scene().camera().project_point(kglt::ViewportID(), kglt::Vec3(0, 0, -10));

        assert_equal(window->width() / 2, p1.x);
        assert_equal(window->height() / 2, p1.y);

        p1 = window->scene().camera().project_point(kglt::ViewportID(), kglt::Vec3(1, 0, -10));

        assert_true(p1.x > (window->width() / 2));
        assert_equal(window->height() / 2, p1.y);
    }

    void test_look_at() {       
        Vec3 pos(0, 0, -1);


        kglt::Stage& stage = window->scene().stage();

        stage.host_camera();
        stage.camera()->look_at(pos);

        kglt::Quaternion q = stage.camera()->absolute_rotation();
        assert_true(kmQuaternionIsIdentity(&q));

        //Just double check that kazmath actually works
        kglt::Mat3 rot;
        kmMat3RotationQuaternion(&rot, &q);

        kglt::Quaternion other;
        kmQuaternionRotationMatrix(&other, &rot);

        assert_true(kmQuaternionAreEqual(&q, &other));

        pos = Vec3(0, -1, 0);
        stage.camera()->look_at(pos);

        assert_equal(Vec3(0, 0, -1), stage.camera()->up());

    }

    void test_following() {
        kglt::Stage& stage = window->scene().stage();
        stage.host_camera();

        ActorID a = stage.new_actor();
        stage.actor(a)->set_absolute_position(kglt::Vec3());

        stage.camera()->follow(a, kglt::Vec3(0, 0, 10));

        assert_equal(Vec3(0, 0, 10), stage.camera()->absolute_position());

        stage.actor(a)->set_absolute_rotation(Degrees(90), 0, -1, 0);
        stage.camera()->_update_following(1.0);

        //FIXME: THis should be closer I think, I'm not sure what's wrong
        assert_equal(Vec3(-10, 0, 0), stage.camera()->absolute_position());
        assert_equal(stage.actor(a)->absolute_rotation(), stage.camera()->absolute_rotation());
    }
};

#endif
