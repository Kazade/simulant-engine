
#include "psp_window.h"
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>


#include "../../sound/drivers/openal_sound_driver.h"
#include "../../sound/drivers/null_sound_driver.h"

#include "../../renderers/renderer_config.h"
#include "../../math/utils.h"

namespace smlt {

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define SCREEN_DEPTH 32


bool PSPWindow::_init_window() {
    set_width(SCREEN_WIDTH);
    set_height(SCREEN_HEIGHT);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    return true;
}

bool PSPWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context
    return true;
}

void PSPWindow::destroy_window() {

}

void PSPWindow::check_events() {
    static bool button_states[JOYSTICK_BUTTON_MAX];
    static bool stick_calibrated = false;
    static uint8_t center_lx = 128;
    static uint8_t center_ly = 128;
    static uint8_t previous_lx = 128;
    static uint8_t previous_ly = 128;

    SceCtrlData pad;
    if(sceCtrlPeekBufferPositive(&pad, 1)) {
        /* The PSP-1000's analog nub frequently rests somewhere other than
         * the nominal (128, 128) centre - sometimes far enough off that it
         * still reads as "pushed" once normalized, even at rest. Rather
         * than assume perfect centring, calibrate against whatever the
         * stick reports on the very first poll (the stick is assumed to
         * be untouched at startup) and treat that as the zero point. */
        if(!stick_calibrated) {
            center_lx = pad.Lx;
            center_ly = pad.Ly;
            previous_lx = pad.Lx;
            previous_ly = pad.Ly;
            stick_calibrated = true;
        }

        auto check_button = [&](PspCtrlButtons psp_button, JoystickButton button) {
            bool pressed = (pad.Buttons & psp_button) == psp_button;
            if(pressed && !button_states[button]) {
                input_state->_handle_joystick_button_down(
                    GameControllerID(0),
                    button
                );

                auto idx = input_state->game_controller_index_from_id(GameControllerID(0));
                on_game_controller_button_down(idx, button);
            } else if(!pressed && button_states[button]) {
                input_state->_handle_joystick_button_up(
                    GameControllerID(0),
                    button
                );

                auto idx = input_state->game_controller_index_from_id(GameControllerID(0));
                on_game_controller_button_up(idx, button);
            }

            button_states[button] = pressed;
        };

        check_button(PSP_CTRL_SELECT, JOYSTICK_BUTTON_GUIDE);
        check_button(PSP_CTRL_START, JOYSTICK_BUTTON_START);
        check_button(PSP_CTRL_UP, JOYSTICK_BUTTON_DPAD_UP);
        check_button(PSP_CTRL_RIGHT, JOYSTICK_BUTTON_DPAD_RIGHT);
        check_button(PSP_CTRL_DOWN, JOYSTICK_BUTTON_DPAD_DOWN);
        check_button(PSP_CTRL_LEFT, JOYSTICK_BUTTON_DPAD_LEFT);
        check_button(PSP_CTRL_LTRIGGER, JOYSTICK_BUTTON_LEFT_SHOULDER);
        check_button(PSP_CTRL_RTRIGGER, JOYSTICK_BUTTON_RIGHT_SHOULDER);
        check_button(PSP_CTRL_TRIANGLE, JOYSTICK_BUTTON_Y);
        check_button(PSP_CTRL_SQUARE, JOYSTICK_BUTTON_X);
        check_button(PSP_CTRL_CIRCLE, JOYSTICK_BUTTON_B);
        check_button(PSP_CTRL_CROSS, JOYSTICK_BUTTON_A);

        // FIXME: Other buttons

        auto handle_axis = [this](uint8_t& previous, uint8_t current, uint8_t center, JoystickAxis axis, bool invert) {
            if(previous == current) {
                return;
            }

            previous = current;

            /* Scale relative to whichever side of centre we're on, so a
             * miscalibrated centre doesn't clip travel in one direction
             * while leaving the other unable to reach -1/+1. */
            float delta = float(current) - float(center);
            float range = (delta >= 0.0f) ? (255.0f - float(center)) : float(center);
            float v = (range > 0.0f) ? clamp(delta / range, -1.0f, 1.0f) : 0.0f;

            if(invert) {
                v = -v;
            }

            input_state->_handle_joystick_axis_motion(GameControllerID(0), axis, v);
        };

        handle_axis(previous_lx, pad.Lx, center_lx, JOYSTICK_AXIS_X, false);
        handle_axis(previous_ly, pad.Ly, center_ly, JOYSTICK_AXIS_Y, true);
    }
}

void PSPWindow::initialize_input_controller(InputState &controller) {
    GameControllerInfo info;
    info.id = GameControllerID(0);
    std::strncpy(info.name, "Internal", sizeof(info.name));
    info.axis_count = 2;
    info.button_count =22;
    info.hat_count = 0;
    info.has_rumble = false;

    controller._update_keyboard_devices({});
    controller._update_mouse_devices({});
    controller._update_game_controllers({info});
}

std::shared_ptr<SoundDriver> PSPWindow::create_sound_driver(const std::string& from_config) {
    const char* from_env = std::getenv("SIMULANT_SOUND_DRIVER");

    std::string selected = (from_env) ? from_env :
        (from_config.empty()) ? "openal" : from_config;

    if(selected == "null") {
        S_DEBUG("Null sound driver activated");
        return std::make_shared<NullSoundDriver>(this);
    } else {
        if(selected != "openal") {
            S_WARN("Unknown sound driver ({0}) falling back to OpenAL", selected);
        }
        S_DEBUG("OpenAL sound driver activated");
        return std::make_shared<OpenALSoundDriver>(this);
    }
}

}
