#pragma once

#include "../deps/kaztest/kaztest/kaztest.h"
#include "../simulant/simulant.h"

namespace {

using namespace smlt;

class PlaneTests : public TestCase {
public:
    void test_distance_to_point() {
        smlt::Vec3 up(0, 1, 0);
        smlt::Plane p(up, 1.0);
        smlt::Vec3 point(0, 2, 0);

        assert_equal(1.0, p.distance_to(point));
    }

    void test_plane_from_point_and_normal() {
        Vec3 p(0, 0, 0);
        Vec3 n(0, 1, 0);

        Plane plane(p, n);

        assert_close(plane.n.x, n.x, 0.00001f);
        assert_close(plane.n.y, n.y, 0.00001f);
        assert_close(plane.n.z, n.z, 0.00001f);
        assert_close(plane.d, 0.0f, 0.00001f);
    }

    void test_point_classification() {

        smlt::Plane p(0, 1, 0, 0);

        smlt::Vec3 in_front(0, 1, 0);
        smlt::Vec3 behind(0, -1, 0);
        smlt::Vec3 on_plane(0, 0, 0);

        assert_true(p.classify_point(in_front) == smlt::PLANE_CLASSIFICATION_IS_IN_FRONT_OF_PLANE);
        assert_false(p.classify_point(in_front) == smlt::PLANE_CLASSIFICATION_IS_BEHIND_PLANE);

        assert_true(p.classify_point(behind) == smlt::PLANE_CLASSIFICATION_IS_BEHIND_PLANE);
        assert_false(p.classify_point(behind) == smlt::PLANE_CLASSIFICATION_IS_IN_FRONT_OF_PLANE);

        assert_true(p.classify_point(on_plane) == smlt::PLANE_CLASSIFICATION_IS_ON_PLANE);
        assert_false(p.classify_point(on_plane) == smlt::PLANE_CLASSIFICATION_IS_BEHIND_PLANE);
        assert_false(p.classify_point(on_plane) == smlt::PLANE_CLASSIFICATION_IS_IN_FRONT_OF_PLANE);
    }
};

}
