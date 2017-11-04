#ifndef TEST_CAMERA_H
#define TEST_CAMERA_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class CameraTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_id_ = window->new_stage();
        camera_id_ = stage_id_.fetch()->new_camera();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        stage_id_.fetch()->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    void test_project_point() {
        auto camera = camera_id_.fetch();
        camera->set_perspective_projection(Degrees(45.0), float(window->width()) / float(window->height()));

        Vec3 p1 = camera->project_point(*window, Viewport(), Vec3(0, 0, -10)).value();

        assert_equal(window->width() / 2, p1.x);
        assert_equal(window->height() / 2, p1.y);

        p1 = camera->project_point(*window, Viewport(), Vec3(1, 0, -10)).value();

        assert_true(p1.x > (window->width() / 2));
        assert_equal(window->height() / 2, p1.y);
    }

    void test_look_at() {       
        Vec3 pos(0, 0, -1);

        auto stage = window->stage(stage_id_);
        stage->camera(camera_id_)->look_at(pos);

        Quaternion q = stage->camera(camera_id_)->absolute_rotation();
        assert_true(q == Quaternion());

        pos = Vec3(0, -1, 0);
        stage->camera(camera_id_)->look_at(pos);

        auto f = stage->camera(camera_id_)->forward();
        assert_close(0.0f, f.x, 0.000001);
        assert_close(-1.0f, f.y, 0.000001);
        assert_close(0.0f, f.z, 0.000001);

        auto res = stage->camera(camera_id_)->up();
        assert_close(res.x, 0, 0.000001);
        assert_close(res.y, 0, 0.000001);
        assert_close(res.z, 1, 0.000001);
    }

    void test_camera_attached_to_parent_moves() {
        auto stage = window->new_stage().fetch();

        auto actor = stage->new_actor().fetch();
        auto camera = stage->new_camera().fetch();

        auto od = camera->frustum().plane(FRUSTUM_PLANE_NEAR).d;

        camera->move_to(0, 0, 10.0f);
        camera->set_parent(actor);

        actor->move_to(0, 0, -10.0f);
        assert_close(camera->absolute_position().z, 0.0f, 0.00001f);

        auto d = camera->frustum().plane(FRUSTUM_PLANE_NEAR).d;
        assert_close(d, od, 0.00001f);
    }

private:
    CameraID camera_id_;
    StageID stage_id_;
};

}

#endif
