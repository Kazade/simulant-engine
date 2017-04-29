#pragma once

#include "simulant/simulant.h"
#include "global.h"

namespace {

class AABBTests : public SimulantTestCase {
public:
    void test_aabb_intersects_aabb() {

        smlt::AABB aabb(smlt::Vec3(), smlt::Vec3(5, 5, 5));
        smlt::AABB aabb2(smlt::Vec3(2.5, 2.5, 2.5), 1.0);
        smlt::AABB aabb3(smlt::Vec3(2.5, 2.5, 2.5), 10.0);

        assert_true(aabb.intersects(aabb2));
        assert_true(aabb2.intersects(aabb3));
    }
};

}
