#pragma once

#include "global.h"
#include "../simulant/input_controller.h"

namespace {

using namespace smlt;

class InputControllerTests : public SimulantTestCase {
public:
    InputController* controller_;

    void set_up() {
        SimulantTestCase::set_up();
        controller_ = new InputController();
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

        assert_equal(controller_->keyboard_count(), 2);

        keyboards.resize(1);

        controller_->_update_keyboard_devices(keyboards);

        assert_equal(controller_->keyboard_count(), 1);
    }

    void test_joystick_hotplugging() {
        std::vector<JoystickDeviceInfo> joysticks(2);
        joysticks[0].id = 0;
        joysticks[1].id = 1;

        controller_->_update_joystick_devices(joysticks);

        assert_equal(controller_->joystick_count(), 2);

        joysticks.resize(1);

        controller_->_update_joystick_devices(joysticks);

        assert_equal(controller_->joystick_count(), 1);
    }

    void test_mouse_hotplugging() {
        std::vector<MouseDeviceInfo> mice(2);
        mice[0].id = 0;
        mice[1].id = 1;

        controller_->_update_mouse_devices(mice);

        assert_equal(controller_->mouse_count(), 2);

        mice.resize(1);

        controller_->_update_mouse_devices(mice);

        assert_equal(controller_->mouse_count(), 1);
    }

    void test_keyboard_key_input() {

    }

    void test_joystick_axis_input() {

    }

    void test_joystick_button_input() {

    }

    void test_mouse_axis_input() {

    }

    void test_mouse_button_input() {

    }

    void test_too_many_keyboards() {

    }
};

}
