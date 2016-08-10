#pragma once

#include "kaztest/kaztest.h"

#include "kglt/kglt.h"
#include "global.h"

class RigidBodyTest : public KGLTTestCase {
public:

    void test_ray_intersections() {
        auto stage_id = window->new_stage();
        auto stage = window->stage(stage_id);

        auto mesh_id = window->shared_assets->new_mesh_as_box(50, 1, 50);

        auto simulation = kglt::controllers::RigidBodySimulation::create();

        auto actor_id = stage->new_actor_with_mesh(mesh_id);
        auto actor = stage->actor(actor_id);
        actor->new_controller<kglt::controllers::StaticBody>(simulation, kglt::controllers::COLLIDER_TYPE_RAYCAST_ONLY);

        auto ret = simulation->intersect_ray(Vec3(0, 5, 0), Vec3(0, -5, 0));
        assert_true(ret.second);
        assert_equal(ret.first.x, 0);
        assert_equal(ret.first.y, 0.5);
        assert_equal(ret.first.z, 0);
    }

};
