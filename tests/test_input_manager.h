#pragma once

#include "../simulant/input/input_manager.h"
#include "../simulant/input/input_axis.h"
#include "../simulant/nodes/ui/keyboard.h"

namespace {

using namespace smlt;

class InputManagerTests : public smlt::test::SimulantTestCase {

public:
    void set_up() {
        SimulantTestCase::set_up();

        state_.reset(new InputState(window));
        state_->init();
        state_->_update_keyboard_devices({KeyboardDeviceInfo{0}});
        state_->_update_game_controllers({GameControllerInfo{GameControllerID(0), "test", 1, 1, 0, false, {0}}});

        manager_.reset(new InputManager(state_.get()));
        manager_->init();

        if(!window->input_state->keyboard_count()) {
            window->input_state->_update_keyboard_devices({KeyboardDeviceInfo{0}});
        }
    }

    void tear_down() {
        manager_->clean_up();
        manager_.reset();

        state_->clean_up();
        state_.reset();
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
        axis->set_joystick_axis(JOYSTICK_AXIS_0);
        axis->set_dead_zone(0.1f);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 0.05f);
        manager_->update(1.0);

        assert_equal(manager_->axis_value_hard("Test"), 0);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 0.11f);
        manager_->update(1.0);

        assert_equal(manager_->axis_value_hard("Test"), 1);
    }

    void test_axis_value_inversion() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_joystick_axis(JOYSTICK_AXIS_0);
        axis->set_dead_zone(0.1f);
        axis->set_inversed(true);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 0.05f);
        manager_->update(1.0);

        assert_equal(manager_->axis_value_hard("Test"), 0);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 0.11f);
        manager_->update(1.0);

        assert_equal(manager_->axis_value_hard("Test"), -1);
    }

    void test_axis_dead_zone() {
        InputAxis* axis = manager_->new_axis("Test");
        axis->set_joystick_axis(JOYSTICK_AXIS_0);
        axis->set_dead_zone(0.1f);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 0.05f);
        manager_->update(1.0);

        assert_equal(axis->value(), 0.0f);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 0.1f);
        manager_->update(1.0);

        assert_equal(axis->value(), 0.0f);

        state_->_handle_joystick_axis_motion(GameControllerID(0), JOYSTICK_AXIS_0, 1.0f);
        manager_->update(1.0);
        assert_close(axis->value(), 1.0f, EPSILON);
    }

    void test_axis_pressed_released() {
        InputAxis* axis = manager_->new_axis("Test");
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

    void test_multiple_axis_pressed_works() {
        InputAxis* axis0 = manager_->new_axis("Test");
        axis0->set_positive_keyboard_key(KEYBOARD_CODE_A);

        InputAxis* axis1 = manager_->new_axis("Test");
        axis1->set_positive_keyboard_key(KEYBOARD_CODE_B);

        assert_false(manager_->axis_was_pressed("Test"));

        state_->_handle_key_down(0, KEYBOARD_CODE_A);
        manager_->update(1.0f);

        assert_true(manager_->axis_was_pressed("Test"));

        state_->_handle_key_up(0, KEYBOARD_CODE_A);
        manager_->update(1.0f);

        assert_false(manager_->axis_was_pressed("Test"));

        state_->_handle_key_down(0, KEYBOARD_CODE_B);
        manager_->update(1.0f);

        assert_true(manager_->axis_was_pressed("Test"));

        state_->_handle_key_down(0, KEYBOARD_CODE_A); // Now press A as well
        manager_->update(1.0f);

        /* State was already set */
        assert_false(manager_->axis_was_pressed("Test"));

        state_->_handle_key_up(0, KEYBOARD_CODE_A);
        state_->_handle_key_up(0, KEYBOARD_CODE_B);
        manager_->update(1.0f);

        assert_true(manager_->axis_was_released("Test"));
    }

    void test_virtual_input() {
        auto& input = window->input;
        auto ret = input->start_text_input(true);
        assert_true(ret);

        unicode text;
        auto conn = input->signal_text_input_received().connect(
            [&](const unicode& c, TextInputEvent&) -> bool {
                text += c;
                return true;
            }
        );

        try {
            input->onscreen_keyboard->cursor_to_char('1');
            input->onscreen_keyboard->activate();
            assert_equal(text, "1");

            auto final_text = input->stop_text_input();
            assert_equal(text, final_text);
        } catch(...) {
            conn.disconnect();
            throw;
        }

        conn.disconnect();
    }

    void test_input_receives_backspace_event() {
        auto& input = window->input;
        input->start_text_input(false);

        bool backspace = false;
        auto conn = input->signal_text_input_received().connect(
            [&](const unicode&, TextInputEvent& evt) -> bool {
                if(evt.keyboard_code == KEYBOARD_CODE_BACKSPACE) {
                    backspace = true;
                }
                return true;
            }
        );

        try {
            window->on_key_down(KEYBOARD_CODE_SPACE, ModifierKeyState());
            assert_false(backspace);

            window->on_key_down(KEYBOARD_CODE_BACKSPACE, ModifierKeyState());
            assert_true(backspace);
        } catch(...) {
            input->stop_text_input();
            conn.disconnect();
            throw;
        }

        input->stop_text_input();
        conn.disconnect();
    }

    void test_start_text_input() {

        auto& input = window->input;
        auto ret = input->start_text_input();

        if(input->state->keyboard_count()) {
            /* We have a physical keyboard, so start_text_input should return false */
            assert_false(ret);
            assert_false(input->onscreen_keyboard_active());
        } else {
            /* On screen keyboard! */
            assert_true(ret);
            assert_true(input->onscreen_keyboard_active());
            input->onscreen_keyboard->cursor_to_char('1');
            input->onscreen_keyboard->activate();
            assert_equal(input->onscreen_keyboard->text(), "1");
            input->onscreen_keyboard->set_text("");
        }

        unicode text;

        auto conn = input->signal_text_input_received().connect(
            [&](const unicode& c, TextInputEvent&) -> bool {
                text += c;
                return true;
            }
        );

        try {
            window->on_key_down(smlt::KEYBOARD_CODE_A, smlt::ModifierKeyState());
            assert_equal(text, "a");

            auto modifier = ModifierKeyState();
            modifier.lshift = true;
            text = "";

            window->on_key_down(smlt::KEYBOARD_CODE_APOSTROPHE, modifier);
            assert_equal(text, "@");

            conn.disconnect();
        } catch(...) {
            conn.disconnect();
            throw;
        }
    }

private:
    std::shared_ptr<InputState> state_;
    std::shared_ptr<InputManager> manager_;
};


}
