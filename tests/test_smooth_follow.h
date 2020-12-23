#ifndef TEST_SMOOTH_FOLLOW_H
#define TEST_SMOOTH_FOLLOW_H

#include "simulant/simulant.h"


namespace {

using namespace smlt;

class SmoothFollowTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage = window->new_stage();
        actor = stage->new_actor();
    }

    void tear_down() {
        SimulantTestCase::tear_down();

        window->destroy_stage(stage->id());
    }

    void test_origin_bug() {
        // See #241

        auto mesh = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_sphere("sphere", stage->assets->new_material(), 10, 5, 5);
        auto sphere = stage->new_actor_with_mesh(mesh);
        auto camera = stage->new_camera();
        auto follow = camera->new_behaviour<smlt::behaviours::SmoothFollow>();
        follow->set_target(sphere);
        follow->set_follow_distance(15.f);
        follow->set_follow_height(10.f);

        window->run_frame();
    }

    void test_target_reset_on_destroy() {
        auto mesh = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_sphere("sphere", stage->assets->new_material(), 10, 5, 5);
        auto sphere = stage->new_actor_with_mesh(mesh);
        auto camera = stage->new_camera();
        auto follow = camera->new_behaviour<smlt::behaviours::SmoothFollow>();
        follow->set_target(sphere);

        assert_true(follow->has_target());
        assert_equal(follow->target(), sphere);

        sphere->destroy();

        assert_false(follow->has_target());
        assert_false(follow->target());
    }

    void test_half_turn() {
        auto follower = stage->new_actor();
        auto controller = follower->new_behaviour<smlt::behaviours::SmoothFollow>();
        controller->set_target(actor);
        controller->set_follow_height(0);

        float step = 1.0f / 60.0f;
        int seconds = 5;

        // Rotate the target 90 degrees
        actor->rotate_to_absolute(Quaternion(Vec3(0, 1, 0), Degrees(90)));

        // Run 5 seconds of updates at 1/60
        for(int32_t i = 0; i < seconds * 60; ++i) {
            controller->_late_update_thunk(step);
        }

        // Follower should now be facing negative Z
        assert_close(
            follower->absolute_rotation().forward().x,
            -1.0f, 0.0001f
        );

        // Rotate the target 180 degrees
        actor->rotate_to_absolute(Quaternion(Vec3(0, 1, 0), Degrees(180)));

        // Run 3 seconds of updates at 1/60
        for(int32_t i = 0; i < seconds * 60; ++i) {
            controller->_late_update_thunk(step);
        }

        // Follower should now be facing positive Z
        assert_close(
            follower->absolute_rotation().forward().z,
            1.0f, 0.0001f
        );

    }

private:
    StagePtr stage;
    ActorPtr actor;
};

}

#endif // TEST_SMOOTH_FOLLOW_H
