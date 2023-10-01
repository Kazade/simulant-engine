#ifndef TEST_CAMERA_H
#define TEST_CAMERA_H

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class CameraTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = scene->create_node<smlt::Stage>();
        camera_ = scene->create_node<smlt::Camera>();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        camera_->destroy();
        stage_->destroy();
    }

    void test_project_point() {
        auto camera = camera_;
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

        camera_->transform->look_at(pos);

        Quaternion q = camera_->transform->orientation();
        assert_true(q == Quaternion());

        pos = Vec3(0, -1, 0);
        camera_->transform->look_at(pos, Vec3(0, 0, -1));

        auto f = camera_->transform->forward();
        assert_close(0.0f, f.x, 0.000001f);
        assert_close(-1.0f, f.y, 0.000001f);
        assert_close(0.0f, f.z, 0.000001f);

        auto res = camera_->transform->up();
        assert_close(res.x, 0.0f, 0.000001f);
        assert_close(res.y, 0.0f, 0.000001f);
        assert_close(res.z, -1.0f, 0.000001f);
    }

    void test_camera_attached_to_parent_moves() {
        auto actor = scene->create_node<smlt::Stage>();
        auto camera = scene->create_node<smlt::Camera>();

        auto od = camera->frustum().plane(FRUSTUM_PLANE_NEAR).d;

        camera->transform->set_translation(Vec3(0, 0, 10.0f));
        camera->set_parent(actor);

        actor->transform->set_translation(Vec3(0, 0, -10.0f));
        assert_close(camera->transform->translation().z, 0.0f, 0.00001f);

        auto d = camera->frustum().plane(FRUSTUM_PLANE_NEAR).d;
        assert_close(d, od, 0.00001f);
    }

private:
    CameraPtr camera_;
    StagePtr stage_;
};

}

#endif
