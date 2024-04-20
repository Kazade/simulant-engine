
#include "psp_window.h"
#include <pspctrl.h>

#include "../../sound/drivers/null_sound_driver.h"
#include "../../renderers/renderer_config.h"

namespace smlt {

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define SCREEN_DEPTH 32

void PSPWindow::swap_buffers() {
    eglSwapBuffers(dpy_, surface_);
}

static const EGLint attrib_list [] = {
    EGL_RED_SIZE, 5,
    EGL_GREEN_SIZE, 6,
    EGL_BLUE_SIZE, 5,
    EGL_ALPHA_SIZE, 0,
    EGL_DEPTH_SIZE, 0,
    EGL_NONE
};

bool PSPWindow::_init_window() {
    dpy_ = eglGetDisplay(0);
    eglInitialize(dpy_, NULL, NULL);

    EGLConfig config;
    EGLint num_configs;

    eglChooseConfig(dpy_, attrib_list, &config, 1, &num_configs);

    if(!num_configs) {
        return false;
    }

    EGLint width, height;

    eglGetConfigAttrib(dpy_, config, EGL_WIDTH, &width);
    eglGetConfigAttrib(dpy_, config, EGL_HEIGHT, &height);

    set_width(width);
    set_height(height);

    ctx_ = eglCreateContext(dpy_, config, NULL, NULL);
    surface_ = eglCreateWindowSurface(dpy_, config, 0, NULL);
    eglMakeCurrent(dpy_, surface_, surface_, ctx_);

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
    eglTerminate(dpy_);
}

void PSPWindow::check_events() {
    static bool button_states[JOYSTICK_BUTTON_MAX];

    SceCtrlData pad;
    if(sceCtrlPeekBufferPositive(&pad, 1)) {
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
    _S_UNUSED(from_config);

    S_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
