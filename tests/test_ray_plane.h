#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class RayPlaneTest: public smlt::test::TestCase {
public:
    void test_intersects_plane_hits() {
        // Ground plane at y=0, ray pointing straight down from above
        Plane ground(Vec3(0, 1, 0), Vec3(0, 0, 0));
        Ray ray(Vec3(0, 10, 0), Vec3(0, -1, 0));

        Vec3 hit;
        float distance;
        bool result = ray.intersects_plane(ground, &hit, &distance);

        assert_true(result);
        assert_close(hit.x, 0.0f, 0.0001f);
        assert_close(hit.y, 0.0f, 0.0001f);
        assert_close(hit.z, 0.0f, 0.0001f);
        assert_close(distance, 10.0f, 0.0001f);
    }

    void test_intersects_plane_behind_ray_misses() {
        Plane ground(Vec3(0, 1, 0), Vec3(0, 0, 0));
        Ray ray(Vec3(0, -10, 0), Vec3(0, -1, 0));

        assert_false(ray.intersects_plane(ground));
    }

    void test_intersects_plane_parallel_misses() {
        Plane ground(Vec3(0, 1, 0), Vec3(0, 0, 0));
        Ray ray(Vec3(0, 5, 0), Vec3(1, 0, 0));

        assert_false(ray.intersects_plane(ground));
    }

    void test_intersects_plane_angled() {
        Plane p(Vec3(1, 0, 0), Vec3(5, 0, 0));
        Ray ray(Vec3(0, 0, 0), Vec3(1, 1, 0));

        Vec3 hit;
        bool result = ray.intersects_plane(p, &hit);

        assert_true(result);
        assert_close(hit.x, 5.0f, 0.0001f);
        assert_close(hit.y, 5.0f, 0.0001f);
    }
};

}
