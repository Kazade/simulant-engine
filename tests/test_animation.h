#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class AnimationTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        animatable_ = std::make_shared<KeyFrameAnimated>();
        animatable_->add_animation("idle", 0, 3, 10.0f);
        animatable_->add_animation("run", 4, 7, 10.0f);

        state_ = std::make_shared<KeyFrameAnimationState>(
            animatable_.get(),
            [this](int32_t current, int32_t next, float interp) {
                last_current_ = current;
                last_next_ = next;
                last_interp_ = interp;
            }
        );
    }

    void tear_down() {
        state_.reset();
        animatable_.reset();
        SimulantTestCase::tear_down();
    }

    void test_default_repeat_is_forever() {
        state_->play_animation("idle");
        assert_equal(state_->current_frame(), 0u);
        assert_equal(state_->next_frame(), 1u);

        // 4 frames (0,1,2,3), each update advances one frame
        float frame_duration = 3.0f / 10.0f;
        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should have wrapped back to frame 0
        assert_equal(state_->current_frame(), 0u);
        assert_equal(state_->next_frame(), 1u);
    }

    void test_repeat_none_stops_after_one_loop() {
        state_->play_animation("idle", ANIMATION_REPEAT_NONE);
        assert_equal(state_->current_frame(), 0u);

        float frame_duration = 3.0f / 10.0f;

        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should have stopped at the last frame (3)
        assert_equal(state_->current_frame(), 3u);
    }

    void test_repeat_forever_loops() {
        state_->play_animation("idle", ANIMATION_REPEAT_FOREVER);
        assert_equal(state_->current_frame(), 0u);

        float frame_duration = 3.0f / 10.0f;

        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        assert_equal(state_->current_frame(), 0u);
        assert_equal(state_->next_frame(), 1u);

        // Advance through another full loop
        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        assert_equal(state_->current_frame(), 0u);
        assert_equal(state_->next_frame(), 1u);
    }

    void test_play_animation_clears_queue() {
        state_->play_animation("idle");
        state_->queue_next_animation("run");

        // Playing a different animation should clear the queue
        state_->play_animation("run");

        float frame_duration = 3.0f / 10.0f;

        // Advance through all frames of run
        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should loop back to run frame 4, not have switched to anything else
        assert_equal(state_->current_frame(), 4u);
    }

    void test_queue_transitions_after_loop_when_repeating() {
        state_->play_animation("idle", ANIMATION_REPEAT_FOREVER);
        state_->queue_next_animation("run");

        float frame_duration = 3.0f / 10.0f;

        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should have transitioned to "run" instead of looping idle
        assert_equal(state_->current_frame(), 4u);
        assert_equal(state_->next_frame(), 5u);
    }

    void test_queue_transitions_after_loop_when_non_repeating() {
        state_->play_animation("idle", ANIMATION_REPEAT_NONE);
        state_->queue_next_animation("run");

        float frame_duration = 3.0f / 10.0f;

        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should have transitioned to "run"
        assert_equal(state_->current_frame(), 4u);
        assert_equal(state_->next_frame(), 5u);
    }

    void test_play_first_animation_uses_default_repeat() {
        state_->play_first_animation();
        assert_equal(state_->current_frame(), 0u);

        float frame_duration = 3.0f / 10.0f;

        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should loop (default is FOREVER)
        assert_equal(state_->current_frame(), 0u);
    }

    void test_play_first_animation_with_repeat_none() {
        state_->play_first_animation(ANIMATION_REPEAT_NONE);
        assert_equal(state_->current_frame(), 0u);

        float frame_duration = 3.0f / 10.0f;

        for(auto i = 0u; i < 4; ++i) {
            state_->update(frame_duration + 0.01f);
        }

        // Should have stopped at the last frame
        assert_equal(state_->current_frame(), 3u);
    }

private:
    std::shared_ptr<KeyFrameAnimated> animatable_;
    std::shared_ptr<KeyFrameAnimationState> state_;

    int32_t last_current_ = -1;
    int32_t last_next_ = -1;
    float last_interp_ = -1.0f;
};

class AnimationControllerTests : public smlt::test::SimulantTestCase {
public:
    void test_current_animation_is_empty_without_animations() {
        auto node = scene->create_child<Stage>();
        auto controller = node->create_mixin<AnimationController>();
        assert_is_not_null(controller);

        assert_false(controller->current_animation());
    }

    void test_current_animation_reports_played_and_queued_animations() {
        auto node = scene->create_child<Stage>();
        auto controller = node->create_mixin<AnimationController>();
        assert_is_not_null(controller);

        Animation idle;
        idle.name = "idle";

        Animation run;
        run.name = "run";

        controller->push_animation(idle);
        controller->push_animation(run);

        assert_true(controller->play("idle"));
        auto current = controller->current_animation();
        assert_true(current);
        assert_equal(current.value(), std::string("idle"));

        // Queueing must not change the current animation until the active
        // one finishes.
        assert_true(controller->queue("run"));
        current = controller->current_animation();
        assert_true(current);
        assert_equal(current.value(), std::string("idle"));

        // Neither animation has channels, so the first update finishes
        // "idle" and advances to the queued animation.
        controller->on_update(0.0f);
        current = controller->current_animation();
        assert_true(current);
        assert_equal(current.value(), std::string("run"));
    }
};

}
