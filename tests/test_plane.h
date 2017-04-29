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

        assert_true(p.classify_point(in_front).is_in_front_of_plane());
        assert_false(p.classify_point(in_front).is_behind_plane());

        assert_true(p.classify_point(behind).is_behind_plane());
        assert_false(p.classify_point(behind).is_in_front_of_plane());

        assert_true(p.classify_point(on_plane).is_on_plane());
        assert_false(p.classify_point(on_plane).is_behind_plane());
        assert_false(p.classify_point(on_plane).is_in_front_of_plane());
    }
};

}
