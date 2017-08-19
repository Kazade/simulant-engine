#pragma once

#include "global.h"
#include "../simulant/input_controller.h"

namespace {

using namespace smlt;

class InputControllerTests : public SimulantTestCase {
public:
    InputState* controller_;

    void set_up() {
        SimulantTestCase::set_up();
        controller_ = new InputState();
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

    void test_joystick_hotplugging() {
        std::vector<JoystickDeviceInfo> joysticks(2);
        joysticks[0].id = 0;
        joysticks[1].id = 1;

        controller_->_update_joystick_devices(joysticks);

        assert_equal(controller_->joystick_count(), 2u);

        joysticks.resize(1);

        controller_->_update_joystick_devices(joysticks);

        assert_equal(controller_->joystick_count(), 1u);
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
        std::vector<JoystickDeviceInfo> joysticks(1);
        joysticks[0].id = 0;
        joysticks[0].axis_count = 2;

        controller_->_update_joystick_devices(joysticks);

        controller_->_handle_joystick_axis_motion(0, JOYSTICK_AXIS_0, 1.0f);

        assert_equal(controller_->joystick_axis_state(0, JOYSTICK_AXIS_X), 1.0f);

        controller_->_handle_joystick_axis_motion(0, JOYSTICK_AXIS_1, -1.0f);

        assert_equal(controller_->joystick_axis_state(0, JOYSTICK_AXIS_Y), -1.0f);
    }

    void test_joystick_button_input() {

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

    }

    void test_too_many_keyboards() {

    }
};

}
