#pragma once


#include "../simulant/input/input_state.h"

namespace {

using namespace smlt;

class InputStateTests : public smlt::test::SimulantTestCase {
public:
    InputState* controller_;

    void set_up() {
        SimulantTestCase::set_up();
        controller_ = new InputState(window);
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        delete controller_;
        controller_ = nullptr;
    }

    void test_keyboard_hotplugging() {
        std::vector<KeyboardDeviceInfo> keyboards(2);
        keyboards[0].id = 0;
        keyboards[1].id = 1;

        controller_->_update_keyboard_devices(keyboards);

        assert_equal(controller_->keyboard_count(), 2u);

        keyboards.resize(1);

        controller_->_update_keyboard_devices(keyboards);

        assert_equal(controller_->keyboard_count(), 1u);
    }

    void test_game_controller_hotplugging() {
        std::vector<GameControllerInfo> game_controllers(2);
        game_controllers[0].id = GameControllerID(0);
        game_controllers[1].id = GameControllerID(1);

        controller_->_update_game_controllers(game_controllers);

        assert_equal(controller_->game_controller_count(), 2u);

        game_controllers = {game_controllers[1]};

        controller_->_update_game_controllers(game_controllers);

        assert_equal(controller_->game_controller_count(), 1u);
        assert_equal(controller_->game_controller(GameControllerIndex(0))->id().to_int8_t(), 1);
        assert_is_null(controller_->game_controller(GameControllerIndex(1)));
    }

    void test_mouse_hotplugging() {
        std::vector<MouseDeviceInfo> mice(2);
        mice[0].id = 0;
        mice[1].id = 1;

        controller_->_update_mouse_devices(mice);

        assert_equal(controller_->mouse_count(), 2u);

        mice.resize(1);

        controller_->_update_mouse_devices(mice);

        assert_equal(controller_->mouse_count(), 1u);
    }

    void test_keyboard_key_input() {

    }

    void test_joystick_axis_input() {
        std::vector<GameControllerInfo> joysticks(1);
        joysticks[0].id = GameControllerID(0);
        joysticks[0].axis_count = 2;

        controller_->_update_game_controllers(joysticks);

        controller_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 1.0f);

        assert_equal(controller_->joystick_axis_state(GameControllerID(0), JOYSTICK_AXIS_X), 1.0f);

        controller_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_1, -1.0f);

        assert_equal(controller_->joystick_axis_state(GameControllerID(0), JOYSTICK_AXIS_Y), -1.0f);
    }

    void test_joystick_button_input() {
        std::vector<GameControllerInfo> joysticks(1);
        joysticks[0].id = GameControllerID(0);
        joysticks[0].button_count = 2;

        controller_->_update_game_controllers(joysticks);

        controller_->_handle_joystick_button_down(GameControllerID(0), JOYSTICK_BUTTON_A);

        assert_true(controller_->joystick_button_state(GameControllerID(0), JOYSTICK_BUTTON_A));

        controller_->_handle_joystick_button_up(GameControllerID(0), JOYSTICK_BUTTON_A);

        assert_false(controller_->joystick_button_state(GameControllerID(0), JOYSTICK_BUTTON_A));
    }

    void test_mouse_axis_input() {
        std::vector<MouseDeviceInfo> mice(2);
        mice[0].id = 0;
        mice[0].axis_count = 2;

        controller_->_update_mouse_devices(mice);

        controller_->_handle_mouse_motion(0, 0, 0, -5, 10);

        assert_equal(controller_->mouse_axis_state(0, MOUSE_AXIS_X), -5.0f);
        assert_equal(controller_->mouse_axis_state(0, MOUSE_AXIS_Y), 10.0f);

        controller_->_handle_mouse_motion(0, 0, 0, 5, -10);

        assert_equal(controller_->mouse_axis_state(0, MOUSE_AXIS_X), 5.0f);
        assert_equal(controller_->mouse_axis_state(0, MOUSE_AXIS_Y), -10.0f);
    }

    void test_mouse_button_input() {
        std::vector<MouseDeviceInfo> mice(2);
        mice[0].id = 0;
        mice[0].axis_count = 2;

        controller_->_update_mouse_devices(mice);

        controller_->_handle_mouse_down(0, 0);

        assert_true(controller_->mouse_button_state(0, 0));

        controller_->_handle_mouse_up(0, 0);

        assert_false(controller_->mouse_button_state(0, 0));
    }

    void test_too_many_keyboards() {

    }
};

}
