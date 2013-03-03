#ifndef TEST_CAMERA_H
#define TEST_CAMERA_H

#include "kglt/kazbase/testing.h"

#include "kglt/kglt.h"
#include "global.h"

class CameraTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_project_point() {
        kglt::SubScene& scene = window->scene().subscene();

        scene.camera().set_perspective_projection(45.0, float(window->width()) / float(window->height()));

        kmVec3 p1 = scene.camera().project_point(kglt::ViewportID(), kglt::Vec3(0, 0, -10));

        this->assert_equal(window->width() / 2, p1.x);
        this->assert_equal(window->height() / 2, p1.y);
    }
};

#endif
