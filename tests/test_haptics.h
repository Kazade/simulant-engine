#pragma once

#include "../simulant/input/input_manager.h"
#include "../simulant/input/input_state.h"
#include "../simulant/window.h"

namespace {

using namespace smlt;

/* A window that does nothing but note the rumble and haptics it's asked
 * for, and can be told whether it has haptics */
class RecordingWindow : public Window {
public:
    bool haptics = false;

    int controller_rumbles = 0;
    int controller_stops = 0;
    float last_low = -1.0f;
    float last_high = -1.0f;

    int haptics_starts = 0;
    int haptics_stops = 0;
    float last_strength = -1.0f;
    float last_duration = -1.0f;

    bool has_haptics() const override {
        return haptics;
    }

    void start_haptics(NormalizedFloat strength, const Seconds& duration) override {
        ++haptics_starts;
        last_strength = strength;
        last_duration = duration.to_float();
    }

    void stop_haptics() override {
        ++haptics_stops;
    }

    void set_title(const std::string&) override {}
    void cursor_position(int32_t&, int32_t&) override {}
    void show_cursor(bool) override {}
    void lock_cursor(bool) override {}
    void check_events() override {}
    void destroy_window() override {}

private:
    void game_controller_start_rumble(GameController*, NormalizedFloat low, NormalizedFloat high, const Seconds& duration) override {
        ++controller_rumbles;
        last_low = low;
        last_high = high;
        last_duration = duration.to_float();
    }

    void game_controller_stop_rumble(GameController*) override {
        ++controller_stops;
    }

    bool _init_window() override { return true; }
    bool _init_renderer(Renderer*) override { return true; }
    void initialize_input_controller(InputState&) override {}

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string&) override {
        return std::shared_ptr<SoundDriver>();
    }
};

class HapticsTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        recorder_.reset(new RecordingWindow());
        state_.reset(new InputState(recorder_.get()));
        state_->init();

        manager_.reset(new InputManager(state_.get()));
        manager_->init();
    }

    void tear_down() {
        manager_->clean_up();
        manager_.reset();
        state_->clean_up();
        state_.reset();
        recorder_.reset();
        SimulantTestCase::tear_down();
    }

    void add_controllers(std::vector<bool> rumble) {
        std::vector<GameControllerInfo> controllers;
        for(std::size_t i = 0; i < rumble.size(); ++i) {
            GameControllerInfo info{GameControllerID(i), "test", 1, 1, 0, rumble[i], {0}};
            controllers.push_back(info);
        }
        state_->_update_game_controllers(controllers);
    }

    void test_windows_have_no_haptics_by_default() {
        /* Only the windows for devices that can vibrate say otherwise */
#ifndef __ANDROID__
        assert_false(window->has_haptics());
#endif
    }

    void test_controllers_rumble() {
        add_controllers({true, true});
        recorder_->haptics = true;

        assert_true(manager_->rumble(0.25f, 0.75f, Seconds(0.5f)));

        assert_equal(recorder_->controller_rumbles, 2);
        assert_close(recorder_->last_low, 0.25f, 0.001f);
        assert_close(recorder_->last_high, 0.75f, 0.001f);
        assert_close(recorder_->last_duration, 0.5f, 0.001f);

        /* With a controller to rumble, the device doesn't vibrate too */
        assert_equal(recorder_->haptics_starts, 0);
    }

    void test_only_controllers_with_rumble_rumble() {
        add_controllers({false, true});

        assert_true(manager_->rumble(0.5f, 0.5f, Seconds(0.1f)));
        assert_equal(recorder_->controller_rumbles, 1);
    }

    void test_device_vibrates_without_controllers() {
        recorder_->haptics = true;

        assert_true(manager_->rumble(0.25f, 0.75f, Seconds(0.3f)));

        assert_equal(recorder_->controller_rumbles, 0);
        assert_equal(recorder_->haptics_starts, 1);

        /* The device has one motor, which gets the stronger of the two */
        assert_close(recorder_->last_strength, 0.75f, 0.001f);
        assert_close(recorder_->last_duration, 0.3f, 0.001f);
    }

    void test_device_vibrates_when_no_controller_can_rumble() {
        add_controllers({false});
        recorder_->haptics = true;

        assert_true(manager_->rumble(0.9f, 0.4f, Seconds(0.2f)));

        assert_equal(recorder_->controller_rumbles, 0);
        assert_equal(recorder_->haptics_starts, 1);
        assert_close(recorder_->last_strength, 0.9f, 0.001f);
    }

    void test_nothing_to_rumble() {
        add_controllers({false});

        assert_false(manager_->rumble(1.0f, 1.0f, Seconds(0.5f)));

        assert_equal(recorder_->controller_rumbles, 0);
        assert_equal(recorder_->haptics_starts, 0);
    }

    void test_stop_rumble() {
        add_controllers({true, false});
        recorder_->haptics = true;

        manager_->stop_rumble();

        /* Only the controller that can rumble is stopped */
        assert_equal(recorder_->controller_stops, 1);
        assert_equal(recorder_->haptics_stops, 1);
    }

    void test_stop_rumble_without_haptics() {
        manager_->stop_rumble();

        assert_equal(recorder_->controller_stops, 0);
        assert_equal(recorder_->haptics_stops, 0);
    }

private:
    std::shared_ptr<RecordingWindow> recorder_;
    std::shared_ptr<InputState> state_;
    std::shared_ptr<InputManager> manager_;
};

}
