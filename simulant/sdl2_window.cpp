//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef __linux__
#include <unistd.h>
#elif defined(__WIN32__)
#include <windows.h>
#include <psapi.h>
#include <sysinfoapi.h>
#include <processthreadsapi.h>
#endif

#include "logging.h"
#include "utils/unicode.h"
#include "input/input_state.h"
#include "sdl2_window.h"
#include "application.h"
#include "sound/drivers/openal_sound_driver.h"
#include "sound/drivers/null_sound_driver.h"

#include "renderers/renderer_config.h"

static const std::string SDL_CONTROLLER_DB =
#include "input/sdl/gamecontrollerdb.txt"
;

namespace smlt {

SDL2Window::SDL2Window() {
    auto default_flags = SDL_INIT_EVERYTHING & (~SDL_INIT_HAPTIC) & (~SDL_INIT_SENSOR);

    if(SDL_Init(default_flags) != 0) {
        S_ERROR("Unable to initialize SDL {0}", SDL_GetError());
        FATAL_ERROR(ERROR_CODE_SDL_INIT_FAILED, "Failed to initialize SDL");
    }

    /* Some platforms don't ship SDL compiled with haptic support, so try it, but don't
     * die if it's not there! */
    if(SDL_InitSubSystem(SDL_INIT_HAPTIC) != 0) {
        S_WARN("Unable to initialize force-feedback. Errors was {0}.", SDL_GetError());
    }

    /* SDL_INIT_SENSOR doesn't always work under some platforms (e.g. Wine) */
    if(SDL_InitSubSystem(SDL_INIT_SENSOR) != 0) {
        S_WARN("Unable to initialize sensor system. Errors was {0}.", SDL_GetError());
    }
}

SDL2Window::~SDL2Window() {
    try {
        _clean_up();
    } catch(...) {
        S_ERROR("There was a problem shutting down the Window. Ignoring.");
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

void SDL2Window::lock_cursor(bool cursor_locked) {
    SDL_SetRelativeMouseMode((cursor_locked) ? SDL_TRUE : SDL_FALSE);
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
            get_app()->stop_running();
        break;
        case SDL_APP_WILLENTERBACKGROUND: {
            std::string sdl_err = SDL_GetError();
            if(!sdl_err.empty()) {
                S_ERROR("Something went wrong with SDL: {0}", sdl_err);
            }

            S_INFO("Application is entering the background, disabling rendering");


            _this->set_has_focus(false);
            {
                //See Window::context_lock_ for details
                thread::Lock<thread::Mutex> context_lock(_this->context_lock());
                _this->set_has_context(false);
            }
        } break;
        case SDL_APP_DIDENTERFOREGROUND: {
            std::string sdl_err = SDL_GetError();
            if(!sdl_err.empty()) {
                S_ERROR("Something went wrong with SDL: " + sdl_err);
            }

            S_INFO("Application is entering the foreground, enabling rendering");
            {
                //See Window::context_lock_ for details
                thread::Lock<thread::Mutex> context_lock(_this->context_lock());
                _this->set_has_context(true);
            }
            //FIXME: Reload textures and shaders
            _this->set_has_focus(true);
        } break;
        default:
            break;
    }

    return 1;
}

JoystickAxis SDL_axis_to_simulant_axis(Uint8 axis) {
    switch(axis) {
    case SDL_CONTROLLER_AXIS_LEFTX: return JOYSTICK_AXIS_0;
    case SDL_CONTROLLER_AXIS_LEFTY: return JOYSTICK_AXIS_1;
    case SDL_CONTROLLER_AXIS_RIGHTX: return JOYSTICK_AXIS_2;
    case SDL_CONTROLLER_AXIS_RIGHTY: return JOYSTICK_AXIS_3;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return JOYSTICK_AXIS_LTRIGGER;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return JOYSTICK_AXIS_RTRIGGER;
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
                get_app()->stop_running();
                break;

            case SDL_CONTROLLERAXISMOTION: {
                auto value = float(event.caxis.value);
                if(event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                    value = clamp(value / float(SDL_JOYSTICK_AXIS_MAX), 0.0f, 1.0f);
                } else {
                    value = clamp(value / float(SDL_JOYSTICK_AXIS_MAX), -1.0f, 1.0f);
                }

                /* For some reason, SDL Y axis is reversed - up == -1, down == 1. I have no idea if this is a bug or correct */
                if(event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY || event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY) {
                    value = -value;
                }

                input_state->_handle_joystick_axis_motion(
                    GameControllerID(event.caxis.which),
                    SDL_axis_to_simulant_axis(event.caxis.axis),
                    value
                );
            } break;
            case SDL_CONTROLLERBUTTONDOWN: {
                input_state->_handle_joystick_button_down(
                    GameControllerID(event.cbutton.which),
                    (JoystickButton) event.cbutton.button
                );
            } break;
            case SDL_CONTROLLERBUTTONUP:
                input_state->_handle_joystick_button_up(
                    GameControllerID(event.cbutton.which),
                    (JoystickButton) event.cbutton.button
                );
            break;
            case SDL_JOYHATMOTION:
                input_state->_handle_joystick_hat_motion(
                    GameControllerID(event.jhat.which),
                    event.jhat.hat, (HatPosition) event.jhat.value
                );
            break;
            case SDL_KEYDOWN: {
                input_state->_handle_key_down(0, (KeyboardCode) event.key.keysym.scancode);
                on_key_down((KeyboardCode) event.key.keysym.scancode, get_modifiers());
            } break;
            case SDL_KEYUP: {
                input_state->_handle_key_up(0, (KeyboardCode) event.key.keysym.scancode);
                on_key_up((KeyboardCode) event.key.keysym.scancode, get_modifiers());
            } break;
            case SDL_MOUSEMOTION: {
                input_state->_handle_mouse_motion(
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

    //FIXME: This should use SDL_GetTouch and a touchID to get the device dimensions
    xout = (int) (x * float(width()));
    yout = (int) (y * float(height()));
}

std::shared_ptr<SoundDriver> SDL2Window::create_sound_driver(const std::string& from_config) {
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

bool SDL2Window::_init_window() {
    /* Load the game controller mappings */
    auto rw_ops = SDL_RWFromConstMem(SDL_CONTROLLER_DB.c_str(), SDL_CONTROLLER_DB.size());
    int ret = SDL_GameControllerAddMappingsFromRW(rw_ops, 1);
    if(ret < 0) {
        S_WARN("Unable to load controller mappings!");
    } else {
        S_DEBUG("Successfully loaded {0} SDL controller mappings", ret);
    }

    int32_t flags = SDL_WINDOW_OPENGL;

    screen_ = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        this->width(), this->height(),
        flags
    );

    if(!screen_) {
        S_ERROR(std::string(SDL_GetError()));
        throw std::runtime_error("FATAL: Unable to create SDL window");
    }

    if(is_fullscreen()) {
        SDL_SetWindowFullscreen(screen_, flags | SDL_WINDOW_FULLSCREEN);
    }

    SDL_SetEventFilter(event_filter, this);

#ifndef NDEBUG
    //DEBUG mode
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
    } else {
#ifdef __ANDROID__
        /* FIXME: Add a GLES2 renderer */
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    context_ = SDL_GL_CreateContext(screen_);

    if(!context_) {
        return false;
    }

    set_has_context(true); //Mark that we have a valid GL context
    SDL_GL_SetSwapInterval((vsync_enabled()) ? 1 : 0);

    return true;
}

void SDL2Window::initialize_input_controller(InputState &controller) {
    std::vector<GameControllerInfo> joypads;

    MouseDeviceInfo mouse;
    mouse.id = 0;
    mouse.button_count = 3; // FIXME: Not always true
    mouse.axis_count = 2; // X + Y FIXME: Add scrollwheels

    KeyboardDeviceInfo keyboard;

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

    controller._update_keyboard_devices({keyboard});
    controller._update_mouse_devices({mouse});
    controller._update_game_controllers(joypads);
}

void SDL2Window::initialize_virtual_screen(uint16_t width, uint16_t height, ScreenFormat format, uint16_t integer_scale) {
    /* Create the virtual screen */
    Screen* screen = _create_screen("virtual", width, height, format, 60);
    screen->_set_integer_scale(integer_scale);
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

void SDL2Window::swap_buffers() {
    SDL_GL_SwapWindow(screen_);
}

bool SDL2Window::initialize_screen(Screen *screen) {
    auto current = SDL_GL_GetCurrentContext();
    auto window = SDL_CreateWindow(
        "Virtual Screen",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screen->width(),
        screen->height(),
        0 //SDL_WINDOW_BORDERLESS
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

void SDL2Window::render_screen(Screen* screen, const uint8_t* data, int row_stride) {
    auto current = SDL_GL_GetCurrentContext();
    auto window = screen->get<SDL_Window*>("window");

    auto surface = SDL_GetWindowSurface(window);

    uint32_t* pixels = (uint32_t*) surface->pixels;

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

void SDL2Window::game_controller_start_rumble(GameControllerID id, uint16_t low_hz, uint16_t high_hz, const Seconds &duration) {
    SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(id.to_int8_t()), low_hz, high_hz, duration.to_float());
}

void SDL2Window::game_controller_stop_rumble(GameControllerID id) {
    SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(id.to_int8_t()), 0, 0, 0);
}


}
