#ifndef TEST_CAMERA_H
#define TEST_CAMERA_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

namespace {

using namespace kglt;

class CameraTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        KGLTTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    void test_project_point() {
        window->camera(camera_id_)->set_perspective_projection(45.0, float(window->width()) / float(window->height()));

        kmVec3 p1 = window->camera(camera_id_)->project_point(*window, kglt::Viewport(), kglt::Vec3(0, 0, -10));

        assert_equal(window->width() / 2, p1.x);
        assert_equal(window->height() / 2, p1.y);

        p1 = window->camera(camera_id_)->project_point(*window, kglt::Viewport(), kglt::Vec3(1, 0, -10));

        assert_true(p1.x > (window->width() / 2));
        assert_equal(window->height() / 2, p1.y);
    }

    void test_look_at() {       
        Vec3 pos(0, 0, -1);


        auto stage = window->stage(stage_id_);

        stage->host_camera(camera_id_);
        stage->camera(camera_id_)->look_at(pos);

        kglt::Quaternion q = stage->camera(camera_id_)->absolute_rotation();
        assert_true(kmQuaternionIsIdentity(&q));

        //Just double check that kazmath actually works
        kglt::Mat3 rot;
        kmMat3FromRotationQuaternion(&rot, &q);

        kglt::Quaternion other;
        kmQuaternionRotationMatrix(&other, &rot);

        assert_true(kmQuaternionAreEqual(&q, &other));

        pos = Vec3(0, -1, 0);
        stage->camera(camera_id_)->look_at(pos);

        assert_equal(Vec3(0, 0, -1), stage->camera(camera_id_)->up());

    }

    void test_following() {
        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);

        ActorID a = stage->new_actor();
        stage->actor(a)->set_absolute_position(kglt::Vec3());

        stage->camera(camera_id_)->follow(a, kglt::CAMERA_FOLLOW_MODE_DIRECT, kglt::Vec3(0, 0, 10));

        assert_equal(Vec3(0, 0, 10), stage->camera(camera_id_)->absolute_position());

        stage->actor(a)->set_absolute_rotation(Degrees(90), 0, -1, 0);
        stage->camera(camera_id_)->_update_following(1.0);

        //FIXME: THis should be closer I think, I'm not sure what's wrong
        assert_equal(Vec3(-10, 0, 0), stage->camera(camera_id_)->absolute_position());
        assert_equal(stage->actor(a)->absolute_rotation(), stage->camera(camera_id_)->absolute_rotation());
    }

private:
    CameraID camera_id_;
    StageID stage_id_;
};

}

#endif
