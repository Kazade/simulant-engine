#pragma once



#include "../simulant/input/input_manager.h"
#include "../simulant/input/input_axis.h"

namespace {

using namespace smlt;

class InputManagerTests : public smlt::test::SimulantTestCase {

public:
    void set_up() {
        SimulantTestCase::set_up();

        state_.reset(new InputState());
        state_->_update_keyboard_devices({KeyboardDeviceInfo{0}});
        state_->_update_joystick_devices({JoystickDeviceInfo{0, "test", 1, 1, 0}});
        manager_.reset(new InputManager(state_.get()));
    }

    void test_axis_force() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_positive_keyboard_key(KEYBOARD_CODE_A);
        state_->_handle_key_down(0, KEYBOARD_CODE_A);
        manager_->update(0.1f);

        // Default is 3.0
        assert_close(axis->value(), 0.3f, EPSILON);
        axis->set_force(1.0f);

        manager_->update(1.0f);
        assert_close(axis->value(), 1.0f, EPSILON);
    }

    void test_axis_return() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_return_speed(0.1f);
        axis->set_positive_keyboard_key(KEYBOARD_CODE_A);

        state_->_handle_key_down(0, KEYBOARD_CODE_A);
        manager_->update(1.0);

        assert_equal(axis->value(), 1.0f);

        state_->_handle_key_up(0, KEYBOARD_CODE_A);

        assert_false(state_->keyboard_key_state(0, KEYBOARD_CODE_A));

        manager_->update(1.0);
        assert_close(axis->value(), 0.9f, 0.0001f);

        manager_->update(1.0);
        assert_close(axis->value(), 0.8f, 0.0001f);
    }

    void test_axis_value_hard() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_type(AXIS_TYPE_JOYSTICK_AXIS);
        axis->set_joystick_axis(JOYSTICK_AXIS_0);
        axis->set_dead_zone(0.1f);

        state_->_handle_joystick_axis_motion(0, JOYSTICK_AXIS_0, 0.05f);
        manager_->update(1.0);

        assert_equal(manager_->axis_value_hard("Test"), 0);

        state_->_handle_joystick_axis_motion(0, JOYSTICK_AXIS_0, 0.1f);
        manager_->update(1.0);

        assert_equal(manager_->axis_value_hard("Test"), 1);
    }

    void test_axis_dead_zone() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_type(AXIS_TYPE_JOYSTICK_AXIS);
        axis->set_joystick_axis(JOYSTICK_AXIS_0);
        axis->set_dead_zone(0.1f);

        state_->_handle_joystick_axis_motion(0, JOYSTICK_AXIS_0, 0.05f);
        manager_->update(1.0);

        assert_equal(axis->value(), 0.0f);

        state_->_handle_joystick_axis_motion(0, JOYSTICK_AXIS_0, 0.1f);
        manager_->update(1.0);

        assert_equal(axis->value(), 0.1f);
    }

    void test_axis_pressed_released() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_type(AXIS_TYPE_KEYBOARD_KEY);
        axis->set_positive_keyboard_key(KEYBOARD_CODE_A);

        assert_false(manager_->axis_was_pressed("Test"));
        assert_false(manager_->axis_was_pressed("TestX")); // Unknown axis

        state_->_handle_key_down(0, KEYBOARD_CODE_0);
        manager_->update(1.0f);

        assert_false(manager_->axis_was_pressed("Test")); // Still false

        state_->_handle_key_down(0, KEYBOARD_CODE_A);
        manager_->update(1.0f);

        assert_true(manager_->axis_was_pressed("Test"));

        state_->_handle_key_up(0, KEYBOARD_CODE_A);
        manager_->update(1.0f);

        assert_false(manager_->axis_was_pressed("Test"));
        assert_true(manager_->axis_was_released("Test"));
    }

private:
    std::shared_ptr<InputState> state_;
    std::shared_ptr<InputManager> manager_;
};


}
