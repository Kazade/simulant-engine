//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "SDL_rwops.h"
#ifdef __linux__
#include <unistd.h>
#elif defined(__WIN32__)
// clang-format off
#include <windows.h>
#include <psapi.h>
#include <sysinfoapi.h>
#include <processthreadsapi.h>
// clang-format on
#endif

#include "../../application.h"
#include "../../input/input_state.h"
#include "../../logging.h"
#include "../../sound/drivers/null_sound_driver.h"
#include "../../sound/drivers/openal_sound_driver.h"
#include "../../vfs.h"
#include "sdl2_window.h"

#include "../../renderers/renderer_config.h"

namespace smlt {

SDL2Window::SDL2Window() {
    auto default_flags =
        SDL_INIT_EVERYTHING & (~SDL_INIT_HAPTIC) & (~SDL_INIT_SENSOR);

    if(SDL_Init(default_flags) != 0) {
        S_ERROR("Unable to initialize SDL {0}", SDL_GetError());
        FATAL_ERROR(ERROR_CODE_SDL_INIT_FAILED, "Failed to initialize SDL");
    }

    /* Some platforms don't ship SDL compiled with haptic support, so try it,
     * but don't die if it's not there! */
    if(SDL_InitSubSystem(SDL_INIT_HAPTIC) != 0) {
        S_WARN("Unable to initialize force-feedback. Errors was {0}.",
               SDL_GetError());
    }

    /* SDL_INIT_SENSOR doesn't always work under some platforms (e.g. Wine) */
    if(SDL_InitSubSystem(SDL_INIT_SENSOR) != 0) {
        S_WARN("Unable to initialize sensor system. Errors was {0}.",
               SDL_GetError());
    }
}

SDL2Window::~SDL2Window() {}

void SDL2Window::set_title(const std::string& title) {
    if(screen_) {
        SDL_SetWindowTitle(screen_, title.c_str());
    }
}

void SDL2Window::show_cursor(bool value) {
    SDL_ShowCursor(value);
}

void SDL2Window::lock_cursor(bool cursor_locked) {
    SDL_SetRelativeMouseMode((cursor_locked) ? SDL_TRUE : SDL_FALSE);
}

void SDL2Window::cursor_position(int32_t& mouse_x, int32_t& mouse_y) {
    SDL_GetMouseState(&mouse_x, &mouse_y);
}

int32_t SDL2Window::default_window_flags() const {
    return SDL_WINDOW_OPENGL;
}

int event_filter(void* user_data, SDL_Event* event) {
    /*
     *  This event filter is for Android, e.g. when someone switches task
     */
    SDL2Window* _this = (SDL2Window*)user_data;

    switch(event->type) {
        case SDL_APP_TERMINATING:
            get_app()->stop_running();
            break;
        case SDL_APP_WILLENTERBACKGROUND: {
            std::string sdl_err = SDL_GetError();
            if(!sdl_err.empty()) {
                S_ERROR("Something went wrong with SDL: {0}", sdl_err);
            }

            S_INFO(
                "Application is entering the background, disabling rendering");

            _this->set_has_focus(false);
            {
                // See Window::context_lock_ for details
                thread::Lock<thread::Mutex> context_lock(_this->context_lock());
                _this->set_has_context(false);
            }
        } break;
        case SDL_APP_DIDENTERFOREGROUND: {
            std::string sdl_err = SDL_GetError();
            if(!sdl_err.empty()) {
                S_ERROR("Something went wrong with SDL: " + sdl_err);
            }

            S_INFO(
                "Application is entering the foreground, enabling rendering");
            {
                // See Window::context_lock_ for details
                thread::Lock<thread::Mutex> context_lock(_this->context_lock());
                _this->set_has_context(true);
            }
            // FIXME: Reload textures and shaders
            _this->set_has_focus(true);
        } break;
        default:
            break;
    }

    return 1;
}

static KeyboardCode scancode_to_keyboard_code(SDL_Scancode code) {
    switch(code) {
        case SDL_SCANCODE_A:
            return KEYBOARD_CODE_A;
        case SDL_SCANCODE_B:
            return KEYBOARD_CODE_B;
        case SDL_SCANCODE_C:
            return KEYBOARD_CODE_C;
        case SDL_SCANCODE_D:
            return KEYBOARD_CODE_D;
        case SDL_SCANCODE_E:
            return KEYBOARD_CODE_E;
        case SDL_SCANCODE_F:
            return KEYBOARD_CODE_F;
        case SDL_SCANCODE_G:
            return KEYBOARD_CODE_G;
        case SDL_SCANCODE_H:
            return KEYBOARD_CODE_H;
        case SDL_SCANCODE_I:
            return KEYBOARD_CODE_I;
        case SDL_SCANCODE_J:
            return KEYBOARD_CODE_J;
        case SDL_SCANCODE_K:
            return KEYBOARD_CODE_K;
        case SDL_SCANCODE_L:
            return KEYBOARD_CODE_L;
        case SDL_SCANCODE_M:
            return KEYBOARD_CODE_M;
        case SDL_SCANCODE_N:
            return KEYBOARD_CODE_N;
        case SDL_SCANCODE_O:
            return KEYBOARD_CODE_O;
        case SDL_SCANCODE_P:
            return KEYBOARD_CODE_P;
        case SDL_SCANCODE_Q:
            return KEYBOARD_CODE_Q;
        case SDL_SCANCODE_R:
            return KEYBOARD_CODE_R;
        case SDL_SCANCODE_S:
            return KEYBOARD_CODE_S;
        case SDL_SCANCODE_T:
            return KEYBOARD_CODE_T;
        case SDL_SCANCODE_U:
            return KEYBOARD_CODE_U;
        case SDL_SCANCODE_V:
            return KEYBOARD_CODE_V;
        case SDL_SCANCODE_W:
            return KEYBOARD_CODE_W;
        case SDL_SCANCODE_X:
            return KEYBOARD_CODE_X;
        case SDL_SCANCODE_Y:
            return KEYBOARD_CODE_Y;
        case SDL_SCANCODE_Z:
            return KEYBOARD_CODE_Z;
        case SDL_SCANCODE_1:
            return KEYBOARD_CODE_1;
        case SDL_SCANCODE_2:
            return KEYBOARD_CODE_2;
        case SDL_SCANCODE_3:
            return KEYBOARD_CODE_3;
        case SDL_SCANCODE_4:
            return KEYBOARD_CODE_4;
        case SDL_SCANCODE_5:
            return KEYBOARD_CODE_5;
        case SDL_SCANCODE_6:
            return KEYBOARD_CODE_6;
        case SDL_SCANCODE_7:
            return KEYBOARD_CODE_7;
        case SDL_SCANCODE_8:
            return KEYBOARD_CODE_8;
        case SDL_SCANCODE_9:
            return KEYBOARD_CODE_9;
        case SDL_SCANCODE_0:
            return KEYBOARD_CODE_0;
        case SDL_SCANCODE_RETURN:
            return KEYBOARD_CODE_RETURN;
        case SDL_SCANCODE_ESCAPE:
            return KEYBOARD_CODE_ESCAPE;
        case SDL_SCANCODE_BACKSPACE:
            return KEYBOARD_CODE_BACKSPACE;
        case SDL_SCANCODE_TAB:
            return KEYBOARD_CODE_TAB;
        case SDL_SCANCODE_SPACE:
            return KEYBOARD_CODE_SPACE;
        case SDL_SCANCODE_MINUS:
            return KEYBOARD_CODE_MINUS;
        case SDL_SCANCODE_EQUALS:
            return KEYBOARD_CODE_EQUALS;
        case SDL_SCANCODE_LEFTBRACKET:
            return KEYBOARD_CODE_LEFTBRACKET;
        case SDL_SCANCODE_RIGHTBRACKET:
            return KEYBOARD_CODE_RIGHTBRACKET;
        case SDL_SCANCODE_BACKSLASH:
            return KEYBOARD_CODE_BACKSLASH;
        case SDL_SCANCODE_NONUSHASH:
            return KEYBOARD_CODE_NONUSHASH;
        case SDL_SCANCODE_SEMICOLON:
            return KEYBOARD_CODE_SEMICOLON;
        case SDL_SCANCODE_APOSTROPHE:
            return KEYBOARD_CODE_APOSTROPHE;
        case SDL_SCANCODE_GRAVE:
            return KEYBOARD_CODE_GRAVE;
        case SDL_SCANCODE_COMMA:
            return KEYBOARD_CODE_COMMA;
        case SDL_SCANCODE_PERIOD:
            return KEYBOARD_CODE_PERIOD;
        case SDL_SCANCODE_SLASH:
            return KEYBOARD_CODE_SLASH;
        case SDL_SCANCODE_CAPSLOCK:
            return KEYBOARD_CODE_CAPSLOCK;
        case SDL_SCANCODE_F1:
            return KEYBOARD_CODE_F1;
        case SDL_SCANCODE_F2:
            return KEYBOARD_CODE_F2;
        case SDL_SCANCODE_F3:
            return KEYBOARD_CODE_F3;
        case SDL_SCANCODE_F4:
            return KEYBOARD_CODE_F4;
        case SDL_SCANCODE_F5:
            return KEYBOARD_CODE_F5;
        case SDL_SCANCODE_F6:
            return KEYBOARD_CODE_F6;
        case SDL_SCANCODE_F7:
            return KEYBOARD_CODE_F7;
        case SDL_SCANCODE_F8:
            return KEYBOARD_CODE_F8;
        case SDL_SCANCODE_F9:
            return KEYBOARD_CODE_F9;
        case SDL_SCANCODE_F10:
            return KEYBOARD_CODE_F10;
        case SDL_SCANCODE_F11:
            return KEYBOARD_CODE_F11;
        case SDL_SCANCODE_F12:
            return KEYBOARD_CODE_F12;
        case SDL_SCANCODE_PRINTSCREEN:
            return KEYBOARD_CODE_PRINTSCREEN;
        case SDL_SCANCODE_SCROLLLOCK:
            return KEYBOARD_CODE_SCROLLLOCK;
        case SDL_SCANCODE_PAUSE:
            return KEYBOARD_CODE_PAUSE;
        case SDL_SCANCODE_INSERT:
            return KEYBOARD_CODE_INSERT;
        case SDL_SCANCODE_HOME:
            return KEYBOARD_CODE_HOME;
        case SDL_SCANCODE_PAGEUP:
            return KEYBOARD_CODE_PAGEUP;
        case SDL_SCANCODE_DELETE:
            return KEYBOARD_CODE_DELETE;
        case SDL_SCANCODE_END:
            return KEYBOARD_CODE_END;
        case SDL_SCANCODE_PAGEDOWN:
            return KEYBOARD_CODE_PAGEDOWN;
        case SDL_SCANCODE_RIGHT:
            return KEYBOARD_CODE_RIGHT;
        case SDL_SCANCODE_LEFT:
            return KEYBOARD_CODE_LEFT;
        case SDL_SCANCODE_DOWN:
            return KEYBOARD_CODE_DOWN;
        case SDL_SCANCODE_UP:
            return KEYBOARD_CODE_UP;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            return KEYBOARD_CODE_NUMLOCKCLEAR;
        case SDL_SCANCODE_KP_DIVIDE:
            return KEYBOARD_CODE_KP_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY:
            return KEYBOARD_CODE_KP_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS:
            return KEYBOARD_CODE_KP_MINUS;
        case SDL_SCANCODE_KP_PLUS:
            return KEYBOARD_CODE_KP_PLUS;
        case SDL_SCANCODE_KP_ENTER:
            return KEYBOARD_CODE_KP_ENTER;
        case SDL_SCANCODE_KP_1:
            return KEYBOARD_CODE_KP_1;
        case SDL_SCANCODE_KP_2:
            return KEYBOARD_CODE_KP_2;
        case SDL_SCANCODE_KP_3:
            return KEYBOARD_CODE_KP_3;
        case SDL_SCANCODE_KP_4:
            return KEYBOARD_CODE_KP_4;
        case SDL_SCANCODE_KP_5:
            return KEYBOARD_CODE_KP_5;
        case SDL_SCANCODE_KP_6:
            return KEYBOARD_CODE_KP_6;
        case SDL_SCANCODE_KP_7:
            return KEYBOARD_CODE_KP_7;
        case SDL_SCANCODE_KP_8:
            return KEYBOARD_CODE_KP_8;
        case SDL_SCANCODE_KP_9:
            return KEYBOARD_CODE_KP_9;
        case SDL_SCANCODE_KP_0:
            return KEYBOARD_CODE_KP_0;
        case SDL_SCANCODE_KP_PERIOD:
            return KEYBOARD_CODE_KP_PERIOD;
        case SDL_SCANCODE_NONUSBACKSLASH:
            return KEYBOARD_CODE_NONUSBACKSLASH;
        case SDL_SCANCODE_APPLICATION:
            return KEYBOARD_CODE_APPLICATION;
        case SDL_SCANCODE_POWER:
            return KEYBOARD_CODE_POWER;
        case SDL_SCANCODE_KP_EQUALS:
            return KEYBOARD_CODE_KP_EQUALS;
        case SDL_SCANCODE_F13:
            return KEYBOARD_CODE_F13;
        case SDL_SCANCODE_F14:
            return KEYBOARD_CODE_F14;
        case SDL_SCANCODE_F15:
            return KEYBOARD_CODE_F15;
        case SDL_SCANCODE_F16:
            return KEYBOARD_CODE_F16;
        case SDL_SCANCODE_F17:
            return KEYBOARD_CODE_F17;
        case SDL_SCANCODE_F18:
            return KEYBOARD_CODE_F18;
        case SDL_SCANCODE_F19:
            return KEYBOARD_CODE_F19;
        case SDL_SCANCODE_F20:
            return KEYBOARD_CODE_F20;
        case SDL_SCANCODE_F21:
            return KEYBOARD_CODE_F21;
        case SDL_SCANCODE_F22:
            return KEYBOARD_CODE_F22;
        case SDL_SCANCODE_F23:
            return KEYBOARD_CODE_F23;
        case SDL_SCANCODE_F24:
            return KEYBOARD_CODE_F24;
        case SDL_SCANCODE_EXECUTE:
            return KEYBOARD_CODE_EXECUTE;
        case SDL_SCANCODE_HELP:
            return KEYBOARD_CODE_HELP;
        case SDL_SCANCODE_MENU:
            return KEYBOARD_CODE_MENU;
        case SDL_SCANCODE_SELECT:
            return KEYBOARD_CODE_SELECT;
        case SDL_SCANCODE_STOP:
            return KEYBOARD_CODE_STOP;
        case SDL_SCANCODE_AGAIN:
            return KEYBOARD_CODE_AGAIN;
        case SDL_SCANCODE_UNDO:
            return KEYBOARD_CODE_UNDO;
        case SDL_SCANCODE_CUT:
            return KEYBOARD_CODE_CUT;
        case SDL_SCANCODE_COPY:
            return KEYBOARD_CODE_COPY;
        case SDL_SCANCODE_PASTE:
            return KEYBOARD_CODE_PASTE;
        case SDL_SCANCODE_FIND:
            return KEYBOARD_CODE_FIND;
        case SDL_SCANCODE_MUTE:
            return KEYBOARD_CODE_MUTE;
        case SDL_SCANCODE_VOLUMEUP:
            return KEYBOARD_CODE_VOLUMEUP;
        case SDL_SCANCODE_VOLUMEDOWN:
            return KEYBOARD_CODE_VOLUMEDOWN;
        case SDL_SCANCODE_KP_COMMA:
            return KEYBOARD_CODE_KP_COMMA;
        case SDL_SCANCODE_KP_EQUALSAS400:
            return KEYBOARD_CODE_KP_EQUALSAS400;
        case SDL_SCANCODE_INTERNATIONAL1:
            return KEYBOARD_CODE_INTERNATIONAL1;
        case SDL_SCANCODE_INTERNATIONAL2:
            return KEYBOARD_CODE_INTERNATIONAL2;
        case SDL_SCANCODE_INTERNATIONAL3:
            return KEYBOARD_CODE_INTERNATIONAL3;
        case SDL_SCANCODE_INTERNATIONAL4:
            return KEYBOARD_CODE_INTERNATIONAL4;
        case SDL_SCANCODE_INTERNATIONAL5:
            return KEYBOARD_CODE_INTERNATIONAL5;
        case SDL_SCANCODE_INTERNATIONAL6:
            return KEYBOARD_CODE_INTERNATIONAL6;
        case SDL_SCANCODE_INTERNATIONAL7:
            return KEYBOARD_CODE_INTERNATIONAL7;
        case SDL_SCANCODE_INTERNATIONAL8:
            return KEYBOARD_CODE_INTERNATIONAL8;
        case SDL_SCANCODE_INTERNATIONAL9:
            return KEYBOARD_CODE_INTERNATIONAL9;
        case SDL_SCANCODE_LANG1:
            return KEYBOARD_CODE_LANG1;
        case SDL_SCANCODE_LANG2:
            return KEYBOARD_CODE_LANG2;
        case SDL_SCANCODE_LANG3:
            return KEYBOARD_CODE_LANG3;
        case SDL_SCANCODE_LANG4:
            return KEYBOARD_CODE_LANG4;
        case SDL_SCANCODE_LANG5:
            return KEYBOARD_CODE_LANG5;
        case SDL_SCANCODE_LCTRL:
            return KEYBOARD_CODE_LCTRL;
        case SDL_SCANCODE_LSHIFT:
            return KEYBOARD_CODE_LSHIFT;
        case SDL_SCANCODE_LALT:
            return KEYBOARD_CODE_LALT;
        case SDL_SCANCODE_LGUI:
            return KEYBOARD_CODE_LGUI;
        case SDL_SCANCODE_RCTRL:
            return KEYBOARD_CODE_RCTRL;
        case SDL_SCANCODE_RSHIFT:
            return KEYBOARD_CODE_RSHIFT;
        case SDL_SCANCODE_RALT:
            return KEYBOARD_CODE_RALT;
        case SDL_SCANCODE_RGUI:
            return KEYBOARD_CODE_RGUI;
        case SDL_SCANCODE_MODE:
            return KEYBOARD_CODE_MODE;
        case SDL_SCANCODE_AUDIONEXT:
            return KEYBOARD_CODE_AUDIONEXT;
        case SDL_SCANCODE_AUDIOPREV:
            return KEYBOARD_CODE_AUDIOPREV;
        case SDL_SCANCODE_AUDIOSTOP:
            return KEYBOARD_CODE_AUDIOSTOP;
        case SDL_SCANCODE_AUDIOPLAY:
            return KEYBOARD_CODE_AUDIOPLAY;
        case SDL_SCANCODE_AUDIOMUTE:
            return KEYBOARD_CODE_AUDIOMUTE;
        case SDL_SCANCODE_MEDIASELECT:
            return KEYBOARD_CODE_MEDIASELECT;
        case SDL_SCANCODE_WWW:
            return KEYBOARD_CODE_WWW;
        case SDL_SCANCODE_MAIL:
            return KEYBOARD_CODE_MAIL;
        case SDL_SCANCODE_CALCULATOR:
            return KEYBOARD_CODE_CALCULATOR;
        case SDL_SCANCODE_COMPUTER:
            return KEYBOARD_CODE_COMPUTER;
        case SDL_SCANCODE_AC_SEARCH:
            return KEYBOARD_CODE_AC_SEARCH;
        case SDL_SCANCODE_AC_HOME:
            return KEYBOARD_CODE_AC_HOME;
        case SDL_SCANCODE_AC_BACK:
            return KEYBOARD_CODE_AC_BACK;
        case SDL_SCANCODE_AC_FORWARD:
            return KEYBOARD_CODE_AC_FORWARD;
        case SDL_SCANCODE_AC_STOP:
            return KEYBOARD_CODE_AC_STOP;
        case SDL_SCANCODE_AC_REFRESH:
            return KEYBOARD_CODE_AC_REFRESH;
        case SDL_SCANCODE_AC_BOOKMARKS:
            return KEYBOARD_CODE_AC_BOOKMARKS;
        case SDL_SCANCODE_BRIGHTNESSDOWN:
            return KEYBOARD_CODE_BRIGHTNESSDOWN;
        case SDL_SCANCODE_BRIGHTNESSUP:
            return KEYBOARD_CODE_BRIGHTNESSUP;
        case SDL_SCANCODE_DISPLAYSWITCH:
            return KEYBOARD_CODE_DISPLAYSWITCH;
        case SDL_SCANCODE_KBDILLUMTOGGLE:
            return KEYBOARD_CODE_KBDILLUMTOGGLE;
        case SDL_SCANCODE_KBDILLUMDOWN:
            return KEYBOARD_CODE_KBDILLUMDOWN;
        case SDL_SCANCODE_KBDILLUMUP:
            return KEYBOARD_CODE_KBDILLUMUP;
        case SDL_SCANCODE_EJECT:
            return KEYBOARD_CODE_EJECT;
        case SDL_SCANCODE_SLEEP:
            return KEYBOARD_CODE_SLEEP;
        case SDL_SCANCODE_APP1:
            return KEYBOARD_CODE_APP1;
        case SDL_SCANCODE_APP2:
            return KEYBOARD_CODE_APP2;
        default:
            return KEYBOARD_CODE_NONE;
    }
}

JoystickAxis SDL_axis_to_simulant_axis(Uint8 axis) {
    switch(axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            return JOYSTICK_AXIS_0;
        case SDL_CONTROLLER_AXIS_LEFTY:
            return JOYSTICK_AXIS_1;
        case SDL_CONTROLLER_AXIS_RIGHTX:
            return JOYSTICK_AXIS_2;
        case SDL_CONTROLLER_AXIS_RIGHTY:
            return JOYSTICK_AXIS_3;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            return JOYSTICK_AXIS_LTRIGGER;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            return JOYSTICK_AXIS_RTRIGGER;
        default:
            throw std::out_of_range("Invalid axis");
    }
}

void SDL2Window::check_events() {
    SDL_Event event;

    auto get_modifiers = []() -> ModifierKeyState {
        ModifierKeyState state;
        auto mod_state = SDL_GetModState();
        state.lctrl = (mod_state & KMOD_LCTRL) == KMOD_LCTRL;
        state.rctrl = (mod_state & KMOD_RCTRL) == KMOD_RCTRL;
        state.lalt = (mod_state & KMOD_LALT) == KMOD_LALT;
        state.ralt = (mod_state & KMOD_RALT) == KMOD_RALT;
        state.lshift = (mod_state & KMOD_LSHIFT) == KMOD_LSHIFT;
        state.rshift = (mod_state & KMOD_RSHIFT) == KMOD_RSHIFT;
        state.lsuper = (mod_state & KMOD_LGUI) == KMOD_LGUI;
        state.rsuper = (mod_state & KMOD_RGUI) == KMOD_RGUI;
        state.mode = (mod_state & KMOD_MODE) == KMOD_MODE;
        state.num_lock = (mod_state & KMOD_NUM) == KMOD_NUM;
        state.caps_lock = (mod_state & KMOD_CAPS) == KMOD_CAPS;
        return state;
    };

    auto to_mouse_button_id = [](uint8_t SDL_button) -> MouseButtonID {
        switch(SDL_button) {
            case SDL_BUTTON_LEFT:
                return 0;
            case SDL_BUTTON_RIGHT:
                return 1;
            case SDL_BUTTON_MIDDLE:
                return 2;
            default:
                return -1;
        }
    };

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                get_app()->stop_running();
                break;
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED: {
                /* If a controller is added or removed, we need to update the
                 * InputState with the new list */
                auto input = get_input_state();
                if(input) {
                    auto controllers = detect_game_controllers();
                    S_DEBUG(
                        "Updating game controller list with {0} controllers",
                        controllers.size());
                    input->_update_game_controllers(controllers);
                }
            } break;
            case SDL_CONTROLLERAXISMOTION: {
                auto value = float(event.caxis.value);
                if(event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
                   event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                    value =
                        clamp(value / float(SDL_JOYSTICK_AXIS_MAX), 0.0f, 1.0f);
                } else {
                    value = clamp(value / float(SDL_JOYSTICK_AXIS_MAX), -1.0f,
                                  1.0f);
                }

                /* For some reason, SDL Y axis is reversed - up == -1, down
                 * == 1. I have no idea if this is a bug or correct */
                if(event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY ||
                   event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY) {
                    value = -value;
                }

                input_state->_handle_joystick_axis_motion(
                    GameControllerID(event.caxis.which),
                    SDL_axis_to_simulant_axis(event.caxis.axis), value);
            } break;
            case SDL_CONTROLLERBUTTONDOWN: {
                input_state->_handle_joystick_button_down(
                    GameControllerID(event.cbutton.which),
                    (JoystickButton)event.cbutton.button);
                auto idx = input_state->game_controller_index_from_id(
                    GameControllerID(event.cbutton.which));
                on_game_controller_button_down(
                    idx, (JoystickButton)event.cbutton.button);
            } break;
            case SDL_CONTROLLERBUTTONUP: {
                input_state->_handle_joystick_button_up(
                    GameControllerID(event.cbutton.which),
                    (JoystickButton)event.cbutton.button);
                auto idx = input_state->game_controller_index_from_id(
                    GameControllerID(event.cbutton.which));
                on_game_controller_button_up(
                    idx, (JoystickButton)event.cbutton.button);
            } break;
            case SDL_JOYHATMOTION:
                input_state->_handle_joystick_hat_motion(
                    GameControllerID(event.jhat.which), event.jhat.hat,
                    (HatPosition)event.jhat.value);
                break;
            case SDL_KEYDOWN: {
                input_state->_handle_key_down(
                    KeyboardID(0),
                    scancode_to_keyboard_code(event.key.keysym.scancode));
                on_key_down(
                    scancode_to_keyboard_code(event.key.keysym.scancode),
                    get_modifiers());
            } break;
            case SDL_KEYUP: {
                input_state->_handle_key_up(
                    KeyboardID(0),
                    scancode_to_keyboard_code(event.key.keysym.scancode));
                on_key_up(scancode_to_keyboard_code(event.key.keysym.scancode),
                          get_modifiers());
            } break;
            case SDL_MOUSEMOTION: {
                bool is_touch_device =
                    (event.motion.which == SDL_TOUCH_MOUSEID);
                input_state->_handle_mouse_motion(
                    MouseID(event.motion.which), event.motion.x,
                    height() - event.motion.y, event.motion.xrel,
                    -event.motion.yrel);
                on_mouse_move(MouseID(event.motion.which), event.motion.x,
                              height() - event.motion.y, is_touch_device);
            } break;
            case SDL_MOUSEBUTTONDOWN: {
                bool is_touch_device =
                    (event.button.which == SDL_TOUCH_MOUSEID);
                input_state->_handle_mouse_down(
                    MouseID(event.button.which),
                    to_mouse_button_id(event.button.button));
                on_mouse_down(MouseID(event.button.which),
                              to_mouse_button_id(event.button.button),
                              event.button.x, height() - event.button.y,
                              is_touch_device);
            } break;
            case SDL_MOUSEBUTTONUP: {
                bool is_touch_device =
                    (event.button.which == SDL_TOUCH_MOUSEID);
                input_state->_handle_mouse_up(
                    MouseID(event.button.which),
                    to_mouse_button_id(event.button.button));
                on_mouse_up(MouseID(event.button.which),
                            to_mouse_button_id(event.button.button),
                            event.button.x, height() - event.button.y,
                            is_touch_device);
            } break;
            case SDL_FINGERMOTION: {
                float x = event.tfinger.x;
                float y = event.tfinger.y;
#ifdef __linux__
                // On Linux, touch coords are window coords not normalized
                // https://bugzilla.libsdl.org/show_bug.cgi?id=2307

                x /= float(this->width());
                y /= float(this->height());
#endif
                on_finger_motion(event.tfinger.fingerId, x, y, event.tfinger.dx,
                                 event.tfinger.dy);
            } break;
            case SDL_FINGERDOWN: {
                float x = event.tfinger.x;
                float y = event.tfinger.y;
#ifdef __linux__
                // On Linux, touch coords are window coords not normalized
                // https://bugzilla.libsdl.org/show_bug.cgi?id=2307

                x /= float(this->width());
                y /= float(this->height());
#endif
                on_finger_down(event.tfinger.fingerId, x, y,
                               event.tfinger.pressure);
            } break;
            case SDL_FINGERUP: {
                float x = event.tfinger.x;
                float y = event.tfinger.y;
#ifdef __linux__
                // On Linux, touch coords are window coords not normalized
                // https://bugzilla.libsdl.org/show_bug.cgi?id=2307

                x /= float(this->width());
                y /= float(this->height());
#endif
                on_finger_up(event.tfinger.fingerId, x, y);
            } break;
            case SDL_WINDOWEVENT: {
                /* Make sure we pause/unpause when the window is minimized and
                 * restored. We also unpause on maximize just in case (although
                 * I imagine that we shouldn't see a maximize without a restore
                 */
                switch(event.window.event) {
                    case SDL_WINDOWEVENT_MINIMIZED:
                        set_has_focus(false);
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        set_has_focus(true);
                        break;
                    default:
                        break;
                }
            } break;
            default:
                S_WARN_ONCE("Unhandled event {0}", event.type);
                break;
        }
    }
}

void SDL2Window::denormalize(float x, float y, int& xout, int& yout) {
    /**
        Given normalized screen coordinates, outputs the absolute position
    */

    // FIXME: This should use SDL_GetTouch and a touchID to get the device
    // dimensions
    xout = (int)(x * float(width()));
    yout = (int)(y * float(height()));
}

std::shared_ptr<SoundDriver>
    SDL2Window::create_sound_driver(const std::string& from_config) {
    const char* from_env = std::getenv("SIMULANT_SOUND_DRIVER");

    std::string selected = (from_env)              ? from_env
                           : (from_config.empty()) ? "openal"
                                                   : from_config;

    if(selected == "null") {
        S_DEBUG("Null sound driver activated");
        return std::make_shared<NullSoundDriver>(this);
    } else {
        if(selected != "openal") {
            S_WARN("Unknown sound driver ({0}) falling back to OpenAL",
                   selected);
        }
        S_DEBUG("OpenAL sound driver activated");
        return std::make_shared<OpenALSoundDriver>(this);
    }
}

bool SDL2Window::_init_window() {
    /* Load the game controller mappings */
    auto controller_db =
        app->vfs->locate_file("assets/other/gamecontrollerdb.txt");

    if(controller_db) {
        int ret = SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile(controller_db.value().str().c_str(), "rb"), 1);

        if(ret < 0) {
            S_WARN("Unable to load controller mappings!");
        } else {
            S_DEBUG("Successfully loaded {0} SDL controller mappings", ret);
        }
    } else {
        S_WARN("Unable to load controller mappings!");
    }

    int32_t flags = default_window_flags();

    if(is_fullscreen()) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    screen_ =
        SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         this->width(), this->height(), flags);

    if(!screen_) {
        S_ERROR(std::string(SDL_GetError()));
        throw std::runtime_error("FATAL: Unable to create SDL window");
    }

    int w, h;
    SDL_GetWindowSize(screen_, &w, &h);
    set_width(w);
    set_height(h);

    SDL_SetEventFilter(event_filter, this);

#ifndef NDEBUG
    // DEBUG mode
    SDL_ShowCursor(1);
#else
    SDL_ShowCursor(0);
#endif

    S_DEBUG("{0} joysicks found", SDL_NumJoysticks());
    for(uint16_t i = 0; i < SDL_NumJoysticks(); i++) {
        if(SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            S_DEBUG(SDL_GameControllerName(controller));
        }
    }

    return true;
}

bool SDL2Window::_init_renderer(Renderer* renderer) {
    if(renderer->name() == "gl1x") {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    } else if(renderer->name() == "gl2x") {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    } else if(renderer->name() == "gles2x") {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    context_ = SDL_GL_CreateContext(screen_);

    if(!context_) {
        return false;
    }

    SDL_GL_MakeCurrent(screen_, context_);
    SDL_GL_SetSwapInterval((vsync_enabled()) ? 1 : 0);

    set_has_context(true); // Mark that we have a valid GL context
    return true;
}

std::vector<GameControllerInfo> SDL2Window::detect_game_controllers() {
    std::vector<GameControllerInfo> joypads;
    for(uint16_t i = 0; i < SDL_NumJoysticks(); i++) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        open_joysticks_.push_back(joystick);

        GameControllerInfo info;
        info.id = GameControllerID(SDL_JoystickInstanceID(joystick));
        std::strncpy(info.name, SDL_JoystickName(joystick), sizeof(info.name));
        info.axis_count = SDL_JoystickNumAxes(joystick);
        info.button_count = SDL_JoystickNumButtons(joystick);
        info.hat_count = SDL_JoystickNumHats(joystick);
        info.has_rumble = SDL_JoystickIsHaptic(joystick);

        joypads.push_back(info);
    }

    return joypads;
}

void SDL2Window::initialize_input_controller(InputState& controller) {

    MouseDeviceInfo mouse;
    mouse.id = 0;
    mouse.button_count = 3; // FIXME: Not always true
    mouse.axis_count = 2;   // X + Y FIXME: Add scrollwheels

    KeyboardDeviceInfo keyboard;
    keyboard.id = 0;

    auto joypads = detect_game_controllers();

    controller._update_keyboard_devices({keyboard});
    controller._update_mouse_devices({mouse});
    controller._update_game_controllers(joypads);
}

bool SDL2Window::initialize_virtual_screen(uint16_t width, uint16_t height,
                                           ScreenFormat format,
                                           uint16_t integer_scale) {
    /* Create the virtual screen */
    Screen* screen = _create_screen("virtual", width, height, format, 60);
    screen->_set_integer_scale(integer_scale);
    return true;
}

void SDL2Window::destroy_window() {
    if(!screen_ && !context_) {
        return;
    }

    for(auto joystick: open_joysticks_) {
        SDL_JoystickClose(joystick);
    }
    open_joysticks_.clear();

    for(auto controller: open_controllers_) {
        SDL_GameControllerClose(controller);
    }
    open_controllers_.clear();

    SDL_Quit();

    context_ = nullptr;
    screen_ = nullptr;
}

void SDL2Window::do_swap_buffers() {
    SDL_GL_SwapWindow(screen_);
}

bool SDL2Window::initialize_screen(Screen* screen) {
    auto current = SDL_GL_GetCurrentContext();
    auto window = SDL_CreateWindow("Virtual Screen", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, screen->width(),
                                   screen->height(),
                                   0 // SDL_WINDOW_BORDERLESS
    );

    if(!window) {
        return false;
    }

    screen->stash(window, "window");

    SDL_GL_MakeCurrent(screen_, current);
    return true;
}

void SDL2Window::shutdown_screen(Screen* screen) {
    SDL_Window* window = screen->get<SDL_Window*>("window");
    if(window) {
        SDL_DestroyWindow(window);
    }
}

void SDL2Window::render_screen(Screen* screen, const uint8_t* data,
                               int row_stride) {
    auto current = SDL_GL_GetCurrentContext();
    auto window = screen->get<SDL_Window*>("window");

    auto surface = SDL_GetWindowSurface(window);

    uint32_t* pixels = (uint32_t*)surface->pixels;

    if(screen->format() == SCREEN_FORMAT_G1) {
        auto x = 0;
        auto y = 0;

        // Get the number of bytes in the data
        auto bytes = (row_stride * screen->height());

        // Go through the bytes
        for(auto i = 0; i < bytes; ++i) {
            // Go through each bit
            for(auto bit = 0; bit < 8; ++bit) {

                // Read the bit value
                auto value = (data[i] >> bit) & 0x01;

                // Work out the pixel offset
                auto offset = ((y * surface->w) + x);

                // Write the pixel at the offset
                pixels[offset] = (value) ? 0 : ~0;

                ++x;
                if(x == screen->width()) {
                    x = 0;
                    y++;
                    i += (row_stride - (screen->width() / 8));
                }
            }
        }
    } else {
        S_ERROR("Unsupported screen format");
        SDL_GL_MakeCurrent(screen_, current);
        return;
    }

    SDL_UpdateWindowSurface(window);
    SDL_GL_MakeCurrent(screen_, current);
}

void SDL2Window::game_controller_start_rumble(GameController* controller,
                                              NormalizedFloat low_rumble,
                                              NormalizedFloat high_rumble,
                                              const Seconds& duration) {
    SDL_GameControllerRumble(
        SDL_GameControllerFromInstanceID(controller->id().to_int8_t()),
        low_rumble * float(0xFFFF), high_rumble * float(0xFFFF),
        duration.to_float() * 1000);
}

void SDL2Window::game_controller_stop_rumble(GameController* controller) {
    SDL_GameControllerRumble(
        SDL_GameControllerFromInstanceID(controller->id().to_int8_t()), 0, 0,
        0);
}

} // namespace smlt
