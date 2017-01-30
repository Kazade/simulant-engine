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
};

}
