#ifndef TEST_SMOOTH_FOLLOW_H
#define TEST_SMOOTH_FOLLOW_H

#include "simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;

class SmoothFollowTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage = window->new_stage();
        actor = stage->new_actor();
    }

    void tear_down() {
        SimulantTestCase::tear_down();

        window->delete_stage(stage->id());
    }


    void test_half_turn() {
        auto follower = stage->new_actor();
        auto controller = follower->new_behaviour<smlt::behaviours::SmoothFollow>();
        controller->set_target(actor);
        controller->set_follow_height(0);

        float step = 1.0 / 60.0f;
        int seconds = 5;

        // Rotate the target 90 degrees
        actor->rotate_to_absolute(Quaternion(Vec3(0, 1, 0), Degrees(90)));

        // Run 3 seconds of updates at 1/60
        for(int32_t i = 0; i < seconds * 60; ++i) {
            controller->_late_update_thunk(step);
        }

        // Follower should now be facing negative Z
        assert_close(
            follower->absolute_rotation().forward().x,
            -1.0, 0.0001
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
            1.0f, 0.0001
        );

    }

private:
    StagePtr stage;
    ActorPtr actor;
};

}

#endif // TEST_SMOOTH_FOLLOW_H
