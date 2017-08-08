//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "deps/kazlog/kazlog.h"
#include "utils/unicode.h"
#include "input_controller.h"
#include "sdl2_window.h"

#include "renderers/renderer_config.h"

namespace smlt {

SDL2Window::SDL2Window(uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen) {
    set_width(width);
    set_height(height);
    set_bpp(bpp);
    set_fullscreen(fullscreen);
}

SDL2Window::~SDL2Window() {
    try {
        _cleanup();
    } catch(...) {
        L_ERROR("There was a problem shutting down the Window. Ignoring.");
    }
}

void SDL2Window::set_title(const std::string& title) {
    if(screen_) {
        SDL_SetWindowTitle(screen_, title.c_str());
    }
}

void SDL2Window::show_cursor(bool value) {
	SDL_ShowCursor(value);
}

void SDL2Window::cursor_position(int32_t& mouse_x, int32_t& mouse_y) {
	SDL_GetMouseState(&mouse_x, &mouse_y);
}

int event_filter(void* user_data, SDL_Event* event) {
    /*
     *  This event filter is for Android, e.g. when someone switches task
     */
    SDL2Window* _this = (SDL2Window*) user_data;

    switch(event->type) {
        case SDL_APP_TERMINATING:
            _this->stop_running();
        break;
        case SDL_APP_WILLENTERBACKGROUND: {
            std::string sdl_err = SDL_GetError();
            if(!sdl_err.empty()) {
                L_ERROR(_F("Something went wrong with SDL: {0}").format(sdl_err));
            }

            L_INFO("Application is entering the background, disabling rendering");


            _this->set_paused(true);
            {
                //See Window::context_lock_ for details
                std::lock_guard<std::mutex> context_lock(_this->context_lock());
                _this->set_has_context(false);
            }
        } break;
        case SDL_APP_DIDENTERFOREGROUND: {
            std::string sdl_err = SDL_GetError();
            if(!sdl_err.empty()) {
                L_ERROR("Something went wrong with SDL: " + sdl_err);
            }

            L_INFO("Application is entering the foreground, enabling rendering");
            {
                //See Window::context_lock_ for details
                std::lock_guard<std::mutex> context_lock(_this->context_lock());
                _this->set_has_context(true);
            }
            //FIXME: Reload textures and shaders
            _this->set_paused(false);
        } break;
        default:
            break;
    }

    return 1;
}

JoypadAxis SDL_axis_to_simulant_axis(Uint8 axis) {
    switch(axis) {
    case SDL_CONTROLLER_AXIS_LEFTX: return JOYPAD_AXIS_LEFT_X;
    case SDL_CONTROLLER_AXIS_LEFTY: return JOYPAD_AXIS_LEFT_Y;
    case SDL_CONTROLLER_AXIS_RIGHTX: return JOYPAD_AXIS_RIGHT_X;
    case SDL_CONTROLLER_AXIS_RIGHTY: return JOYPAD_AXIS_RIGHT_Y;
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


    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                stop_running();
                break;

            case SDL_JOYAXISMOTION:
                input_controller()._handle_joypad_axis_motion(
                    event.jaxis.which, SDL_axis_to_simulant_axis(event.jaxis.axis), event.jaxis.value
                );
            break;
            case SDL_JOYBUTTONDOWN:
                input_controller()._handle_joypad_button_down(event.jbutton.which, event.jbutton.button);
            break;
            case SDL_JOYBUTTONUP:
                input_controller()._handle_joypad_button_up(event.jbutton.which, event.jbutton.button);
            break;
            case SDL_JOYHATMOTION:
                input_controller()._handle_joypad_hat_motion(event.jhat.which, event.jhat.hat, (HatPosition) event.jhat.value);
            break;
            case SDL_KEYDOWN: {
                input_controller()._handle_key_down(0, (KeyboardCode) event.key.keysym.scancode);
                on_key_down((KeyboardCode) event.key.keysym.scancode, get_modifiers());
            } break;
            case SDL_KEYUP: {
                input_controller()._handle_key_up(0, (KeyboardCode) event.key.keysym.scancode);
                on_key_up((KeyboardCode) event.key.keysym.scancode, get_modifiers());
            } break;
            case SDL_MOUSEMOTION: {
                input_controller()._handle_mouse_motion(
                    event.motion.which,
                    event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel
                );
            } break;
            case SDL_MOUSEBUTTONDOWN: {

            } break;
            case SDL_MOUSEBUTTONUP: {

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
                on_finger_motion(
                    event.tfinger.fingerId, x, y, event.tfinger.dx, event.tfinger.dy
                );
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
                on_finger_down(
                    event.tfinger.fingerId, x, y, event.tfinger.pressure
                );
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
                on_finger_up(
                    event.tfinger.fingerId, x, y
                );
            } break;
            case SDL_WINDOWEVENT: {
                /* Make sure we pause/unpause when the window is minimized and restored.
                 * We also unpause on maximize just in case (although I imagine that we shouldn't
                 * see a maximize without a restore */
                switch(event.window.event) {
                    case SDL_WINDOWEVENT_MINIMIZED:
                        set_paused(true);
                    break;
                    case SDL_WINDOWEVENT_RESTORED:
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        set_paused(false);
                    break;
                    default:
                        break;
                }
            } break;
            default:
                L_WARN_ONCE(_F("Unhandled event {0}").format(event.type));
                break;
        }
    }
}

void SDL2Window::denormalize(float x, float y, int& xout, int& yout) {
    /**
        Given normalized screen coordinates, outputs the absolute position
    */

    //FIXME: This should use SDL_GetTouch and a touchID to get the device dimensions
    xout = (int) (x * float(width()));
    yout = (int) (y * float(height()));
}

bool SDL2Window::create_window(int width, int height, int bpp, bool fullscreen) {
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        L_ERROR(_F("Unable to initialize SDL {0}").format(SDL_GetError()));
        return false;
    }

    int32_t flags = SDL_WINDOW_OPENGL;
    if(fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

#ifdef SIMULANT_GL_VERSION_1X
    renderer_ = std::make_shared<GL1XRenderer>(this);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 5);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#elif __ANDROID__
    renderer_ = std::make_shared<GenericRenderer>(this);

    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    renderer_ = std::make_shared<GenericRenderer>(this);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    screen_ = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height,
        flags
    );

    if(!screen_) {
        L_ERROR(std::string(SDL_GetError()));
        throw std::runtime_error("FATAL: Unable to create SDL window");
    }

    context_ = SDL_GL_CreateContext(screen_);

    if(!context_) {
        throw std::runtime_error("FATAL: Unable to create a GL context");
    }

    SDL_SetEventFilter(event_filter, this);
    set_has_context(true); //Mark that we have a valid GL context

    renderer_->init_context();

    //Reset the width and height to whatever was actually created
    SDL_GetWindowSize(screen_, &width, &height);

    set_width(width);
    set_height(height);

#ifndef NDEBUG
    //DEBUG mode
    SDL_GL_SetSwapInterval(0);
    SDL_ShowCursor(1);
#else
    SDL_ShowCursor(0);
#endif


    L_DEBUG(unicode("{0} joysicks found").format(SDL_NumJoysticks()).encode());
    for(uint16_t i = 0; i < SDL_NumJoysticks(); i++) {
        if(SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            L_DEBUG(SDL_GameControllerName(controller));
        }
    }

    return true;
}

void SDL2Window::initialize_input_controller(InputController &controller) {
    std::vector<GameControllerInfo> joypads;

    MouseControllerInfo mouse;
    mouse.id = 0;
    mouse.button_count = 3;

    KeyboardControllerInfo keyboard;

    for(uint16_t i = 0; i < SDL_NumJoysticks(); i++) {
        if(SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);

            GameControllerInfo info;
            info.id = i;
            info.name = SDL_GameControllerName(controller);

            joypads.push_back(info);
            open_controllers_.push_back(controller);
        } else {
            SDL_Joystick* joystick = SDL_JoystickOpen(i);

            GameControllerInfo info;
            info.id = i;
            info.name = SDL_JoystickName(joystick);

            joypads.push_back(info);
            open_joysticks_.push_back(joystick);
        }
    }

    controller._update_keyboard_devices({keyboard});
    controller._update_mouse_devices({mouse});
    controller._update_joypad_devices(joypads);
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

    SDL_GL_DeleteContext(context_);
    context_ = nullptr;

    SDL_DestroyWindow(screen_);
    screen_ = nullptr;

    SDL_Quit();
}

void SDL2Window::swap_buffers() {
    SDL_GL_SwapWindow(screen_);
}

}
