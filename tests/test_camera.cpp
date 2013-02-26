#include <UnitTest++.h>

#include "kglt/kglt.h"
#include "kglt/testing/mock_window.h"

using kglt::testing::MockWindow;

TEST(test_project_point) {
    MockWindow::ptr window = MockWindow::create();
    kglt::SubScene& scene = window->scene().subscene();

    scene.camera().set_perspective_projection(45.0, float(window->width()) / float(window->height()));

    kmVec3 p1 = scene.camera().project_point(kglt::ViewportID(), kglt::Vec3(0, 0, -10));

    CHECK_EQUAL(window->width() / 2, p1.x);
    CHECK_EQUAL(window->height() / 2, p1.y);
}



