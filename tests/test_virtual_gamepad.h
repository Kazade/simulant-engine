#ifndef TEST_VIRTUAL_GAMEPAD_H
#define TEST_VIRTUAL_GAMEPAD_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

namespace {

using namespace kglt;

class VirtualGamepadTest : public KGLTTestCase {
private:
    bool b1_pressed = false;
    bool b2_pressed = false;

    sig::connection conn1;
    sig::connection conn2;

public:
    void set_up() {
        KGLTTestCase::set_up();

        b1_pressed = false;
        b2_pressed = false;

        window->enable_virtual_joypad(VIRTUAL_DPAD_DIRECTIONS_TWO, 2);

        conn1 = window->virtual_joypad()->signal_button_up().connect([&](int i) {
            if(i == 0) {
                b1_pressed = false;
            } else {
                b2_pressed = false;
            }
        });

        conn2 = window->virtual_joypad()->signal_button_down().connect([&](int i) {
            if(i == 0) {
                b1_pressed = true;
            } else {
                b2_pressed = true;
            }
        });


    }

    void tear_down() {
        window->disable_virtual_joypad();
        conn1.disconnect();
        conn2.disconnect();

        KGLTTestCase::tear_down();
    }

    void test_touchdown_event_triggers_signal() {
        auto b1 = window->virtual_joypad()->button_dimensions(0);
        auto b2 = window->virtual_joypad()->button_dimensions(1);

        int x = b1.left + 1;
        int y = b1.top + 1;

        window->handle_touch_down(0, x, y);

        assert_true(b1_pressed);
        assert_false(b2_pressed);

        x = b2.left + 1;
        y = b2.top - 1; //Outside the button

        window->handle_touch_down(1, x, y);
        assert_false(b2_pressed);
        assert_true(b1_pressed);

        x = b2.left + 1;
        y = b2.top + 1;

        window->handle_touch_down(1, x, y);
        assert_true(b2_pressed);
        assert_true(b1_pressed);

        window->handle_touch_motion(1, x, y);
        assert_true(b2_pressed);
        assert_true(b1_pressed);

        window->handle_touch_motion(1, 0, 0);
        assert_false(b2_pressed);
        assert_true(b1_pressed);
    }

    void test_touchup_event_triggers_signal() {
        auto b1 = window->virtual_joypad()->button_dimensions(0);
        auto b2 = window->virtual_joypad()->button_dimensions(1);


        int x = b1.left + 1;
        int y = b1.top + 1;

        window->handle_touch_down(0, x, y);

        x = b2.left + 1;
        y = b2.top + 1;

        window->handle_touch_down(1, x, y);

        // We should have pressed both buttons, one with each finger
        assert_true(b1_pressed);
        assert_true(b2_pressed);

        // The location of the touch up event doesn't matter,
        // if the finger is released, the button should be released
        window->handle_touch_up(0, 0, 0);

        assert_false(b1_pressed);
        assert_true(b2_pressed);

        window->handle_touch_up(1, 0, 0);
        assert_false(b2_pressed);

    }

    void test_button_released_when_all_touches_are_finished() {
        /*
         *  This is a tricky one. If someone presses a button with finger 0, then finger 1,
         *  then releases finger 1, the button should remain pressed
         */

        auto b1 = window->virtual_joypad()->button_dimensions(0);


        int x = b1.left + 1;
        int y = b1.top + 1;

        window->handle_touch_down(0, x, y);
        window->handle_touch_down(1, x, y);

        assert_true(b1_pressed);

        window->handle_touch_up(1, 0, 0);

        assert_true(b1_pressed);

        window->handle_touch_up(0, 0, 0);

        assert_false(b2_pressed);
    }

    void test_mouse_clicks_dont_interfere() {
        auto b1 = window->virtual_joypad()->button_dimensions(0);

        int x = b1.left + 1;
        int y = b1.top + 1;

        window->handle_mouse_motion(x, y);
        window->handle_mouse_button_down(0);
        window->handle_touch_down(0, x, y);

        assert_true(b1_pressed);

        window->handle_mouse_button_up(0);

        assert_true(b1_pressed);

        window->handle_touch_up(0, x, y);

        assert_false(b1_pressed);
    }

    void test_deactivation_releases_buttons() {
        auto b1 = window->virtual_joypad()->button_dimensions(0);

        int x = b1.left + 1;
        int y = b1.top + 1;

        window->handle_touch_down(0, x, y);

        assert_true(b1_pressed);

        window->disable_virtual_joypad();

        assert_false(b1_pressed);
    }
};

class VirtualGamepadInputTests : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        window->enable_virtual_joypad(VIRTUAL_DPAD_DIRECTIONS_TWO, 2);
    }

    void tear_down() {
        KGLTTestCase::tear_down();
    }

    void test_input_controller_signals_fire() {
        bool button_pressed = false;

        int virtual_joypad = window->joypad_count() - 1;
        window->joypad(virtual_joypad).button_pressed_connect(0, [&](int btn) {
            assert_equal(0, btn);
            button_pressed = true;
        });

        auto b1 = window->virtual_joypad()->button_dimensions(0);

        int x = b1.left + 1;
        int y = b1.top + 1;

        window->handle_touch_down(0, x, y);

        assert_true(button_pressed);
    }

    void test_while_down() {
        bool button_pressed = false;

        auto b1 = window->virtual_joypad()->button_dimensions(0);

        int x = b1.left + 1;
        int y = b1.top + 1;

        window->joypad(window->joypad_count() - 1).button_while_down_connect(0, [&](int btn, double dt) {
            button_pressed = true;
            window->handle_touch_up(0, x, y);
        });

        window->handle_touch_down(0, x, y);
        window->run_frame();

        assert_true(button_pressed);
    }
};

}


#endif // TEST_VIRTUAL_GAMEPAD_H
