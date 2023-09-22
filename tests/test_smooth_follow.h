#ifndef TEST_SMOOTH_FOLLOW_H
#define TEST_SMOOTH_FOLLOW_H

#include "simulant/simulant.h"


namespace {

using namespace smlt;

class SmoothFollowTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage = scene->create_node<smlt::Stage>();
        actor = scene->create_node<smlt::Actor>(
            scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT)
        );
    }

    void tear_down() {
        SimulantTestCase::tear_down();

        stage->destroy();
    }

    void test_origin_bug() {
        // See #241

        auto mesh = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_sphere("sphere", scene->assets->new_material(), 10, 5, 5);
        auto sphere = scene->create_node<smlt::Actor>(mesh);
        auto follow = scene->create_node<smlt::SmoothFollow>();
        follow->set_target(sphere);
        follow->set_follow_distance(15.f);
        follow->set_follow_height(10.f);

        application->run_frame();
    }

    void test_target_reset_on_destroy() {
        auto mesh = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_sphere("sphere", scene->assets->new_material(), 10, 5, 5);
        auto sphere = scene->create_node<smlt::Actor>(mesh);
        auto follow = scene->create_node<smlt::SmoothFollow>();
        follow->set_target(sphere);

        assert_true(follow->has_target());
        assert_equal(follow->target(), sphere);

        sphere->destroy();

        assert_false(follow->has_target());
        assert_false(follow->target());
    }

    void test_half_turn() {
        auto follower = scene->create_node<smlt::SmoothFollow>();
        follower->set_target(actor);
        follower->set_follow_height(0);

        float step = 1.0f / 60.0f;
        int seconds = 5;

        // Rotate the target 90 degrees
        actor->rotate_to_absolute(Quaternion(Vec3(0, 1, 0), Degrees(90)));

        // Run 5 seconds of updates at 1/60
        for(int32_t i = 0; i < seconds * 60; ++i) {
            follower->late_update(step);
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
            follower->late_update(step);
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
