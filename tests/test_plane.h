#pragma once

#include "../deps/kaztest/kaztest/kaztest.h"
#include "../simulant/simulant.h"

namespace {

class PlaneTests : public TestCase {
public:
    void test_distance_to_point() {
        smlt::Vec3 up(0, 1, 0);
        smlt::Plane p(up, 1.0);
        smlt::Vec3 point(0, 2, 0);

        assert_equal(1.0, p.distance_to(point));
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
