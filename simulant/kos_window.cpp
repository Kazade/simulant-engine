
#include <malloc.h>
#include <memory>
#include <vector>
#include "../deps/libgl/include/GL/gl.h"
#include "../deps/libgl/include/GL/glkos.h"

#include "input/input_state.h"
#include "kos_window.h"

#include "sound/drivers/openal_sound_driver.h"
#include "sound/drivers/null_sound_driver.h"

#include "renderers/renderer_config.h"

namespace smlt {

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS | INIT_NET);

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_DEPTH 32


void KOSWindow::swap_buffers() {
    glKosSwapBuffers();
}

bool KOSWindow::_init_window() {
    S_DEBUG("Initializing OpenGL");

    static bool gl_initialized = false;
    if(!gl_initialized) {
        glKosInit();
        gl_initialized = true;
    }

    S_DEBUG("OpenGL initialized");
    return true;
}

bool KOSWindow::_init_renderer(Renderer* renderer) {
    set_has_context(true); //Mark that we have a valid GL context

    S_DEBUG("Renderer initialized");

    return true;
}

void KOSWindow::destroy_window() {

}

void KOSWindow::probe_vmus() {
    const static char LETTERS [] = {'A', 'B', 'C', 'D'};

    const static int MAX_VMUS = 8;  // Four ports, two slots in each

    thread::Lock<thread::Mutex> g(vmu_mutex_);

    vmu_lookup_.clear();

    for(auto i = 0; i < MAX_VMUS; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_LCD);
        if(device && device->valid) {
            std::string identifier = _F("{0}{1}").format(
                LETTERS[device->port],
                device->unit
            );

            vmu_lookup_[identifier] = std::make_pair(device->port, device->unit);
        }
    }
}

static constexpr JoystickButton dc_button_to_simulant_button(uint16_t dc_button) {
    return (dc_button == CONT_A) ? JOYSTICK_BUTTON_A :
           (dc_button == CONT_B) ? JOYSTICK_BUTTON_B :
           (dc_button == CONT_C) ? JOYSTICK_BUTTON_LEFT_SHOULDER :
           (dc_button == CONT_X) ? JOYSTICK_BUTTON_X :
           (dc_button == CONT_Y) ? JOYSTICK_BUTTON_Y :
           (dc_button == CONT_Z) ? JOYSTICK_BUTTON_RIGHT_SHOULDER :
           (dc_button == CONT_START) ? JOYSTICK_BUTTON_START :
           JOYSTICK_BUTTON_INVALID;
}

void KOSWindow::check_events() {
    probe_vmus();

    const int8_t MAX_CONTROLLERS = 4;
    const static std::vector<uint16_t> CONTROLLER_BUTTONS = {
        CONT_A, CONT_B, CONT_C, CONT_D, CONT_X, CONT_Y, CONT_Z, CONT_START
    };

    const static std::vector<uint16_t> HAT1_BUTTONS = {
        CONT_DPAD_UP, CONT_DPAD_DOWN, CONT_DPAD_LEFT, CONT_DPAD_RIGHT
    };

    const static std::vector<uint16_t> HAT2_BUTTONS = {
        CONT_DPAD2_UP, CONT_DPAD2_DOWN, CONT_DPAD2_LEFT, CONT_DPAD2_RIGHT
    };

    static std::array<uint32_t, MAX_CONTROLLERS> previous_controller_button_state = {{0, 0, 0, 0}};
    static std::array<HatPosition, MAX_CONTROLLERS> previous_hat1_state = {
        {HAT_POSITION_CENTERED, HAT_POSITION_CENTERED, HAT_POSITION_CENTERED, HAT_POSITION_CENTERED}
    };
    static std::array<HatPosition, MAX_CONTROLLERS> previous_hat2_state = {
        {HAT_POSITION_CENTERED, HAT_POSITION_CENTERED, HAT_POSITION_CENTERED, HAT_POSITION_CENTERED}
    };

    static std::array<uint8_t, 256> previous_key_state = {}; // value-initialize to zero
    
    struct ControllerState {
        int8_t joyx = 0;
        int8_t joyy = 0;
        int8_t joyx2 = 0;
        int8_t joyy2 = 0;
        uint8_t ltrig = 0;
        uint8_t rtrig = 0;
    };
    
    static std::array<ControllerState, MAX_CONTROLLERS> previous;
    
    // Rescan for devices in case a controller has been added or removed
    initialize_input_controller(*this->_input_state());

    /* Check controller states */
    for(int8_t i = 0; i < MAX_CONTROLLERS; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if(device) {
            auto state = (cont_state_t*) maple_dev_status(device);
            if(state) {
                auto button_state = state->buttons;
                auto prev_state = previous_controller_button_state[i];
                auto joyx_state = state->joyx;
                auto joyy_state = state->joyy;
                auto joyx2_state = state->joy2x;
                auto joyy2_state = state->joy2y;
                auto ltrig_state = state->ltrig;
                auto rtrig_state = state->rtrig;
                
                auto handle_joystick_axis = [this](int prev, int current, int controller, smlt::JoystickAxis target, float range=127.0f) -> int16_t {
                    if(prev == current) {
	                    return current;
                    }
                    
                    float v = float(current) / range;
                    
                    if(target == JOYSTICK_AXIS_LTRIGGER || target == JOYSTICK_AXIS_RTRIGGER) {
                        v = clamp(v, 0.0f, 1.0f);
                    } else {
                        v = clamp(v, -1.0f, 1.0f);
                    }

                    if(target == JOYSTICK_AXIS_YL || target == JOYSTICK_AXIS_YR) {
                        v = -v;
                    }

                    input_state->_handle_joystick_axis_motion(controller, target, v);
                    return current;
                };
			    
                previous[i].joyx = handle_joystick_axis(previous[i].joyx, joyx_state, i, JOYSTICK_AXIS_X);
                previous[i].joyy = handle_joystick_axis(previous[i].joyy, joyy_state, i, JOYSTICK_AXIS_Y);
                previous[i].joyx2 = handle_joystick_axis(previous[i].joyx2, joyx2_state, i, JOYSTICK_AXIS_2);
                previous[i].joyy2 = handle_joystick_axis(previous[i].joyy2, joyy2_state, i, JOYSTICK_AXIS_3);				
                previous[i].ltrig = handle_joystick_axis(previous[i].ltrig, ltrig_state, i, JOYSTICK_AXIS_4, 255.0f);	
                previous[i].rtrig = handle_joystick_axis(previous[i].rtrig, rtrig_state, i, JOYSTICK_AXIS_5, 255.0f);					
            
                // Check the current button state against the previous one
                // and update the input controller appropriately
                for(auto button: CONTROLLER_BUTTONS) {
                    bool prev = prev_state & button;
                    bool curr = button_state & button;

                    if(!prev && curr) {
                        // Button down
                        input_state->_handle_joystick_button_down(
                            i, dc_button_to_simulant_button(button)
                        );
                    } else if(prev && !curr) {
                        // Button up
                        input_state->_handle_joystick_button_up(
                            i, dc_button_to_simulant_button(button)
                        );
                    }
                }

                uint32_t hat1_state = (uint32_t) HAT_POSITION_CENTERED;
                for(auto button: HAT1_BUTTONS) {
                    if(button_state & button) {
                        switch(button) {
                        case CONT_DPAD_UP:
                            hat1_state |= (uint32_t) HAT_POSITION_UP;
                        break;
                        case CONT_DPAD_DOWN:
                            hat1_state |= (uint32_t) HAT_POSITION_DOWN;
                        break;
                        case CONT_DPAD_LEFT:
                            hat1_state |= (uint32_t) HAT_POSITION_LEFT;
                        break;
                        case CONT_DPAD_RIGHT:
                            hat1_state |= (uint32_t) HAT_POSITION_RIGHT;
                        break;
                        default:
                            break;
                        }
                    }
                }

                if(hat1_state != previous_hat1_state[i]) {
                    input_state->_handle_joystick_hat_motion(i, 0, (HatPosition) hat1_state);
                    previous_hat1_state[i] = (HatPosition) hat1_state;
                }

                uint32_t hat2_state = (uint32_t) HAT_POSITION_CENTERED;
                for(auto button: HAT2_BUTTONS) {
                    if(button_state & button) {
                        switch(button) {
                        case CONT_DPAD2_UP:
                            hat2_state |= (uint32_t) HAT_POSITION_UP;
                        break;
                        case CONT_DPAD2_DOWN:
                            hat2_state |= (uint32_t) HAT_POSITION_DOWN;
                        break;
                        case CONT_DPAD2_LEFT:
                            hat2_state |= (uint32_t) HAT_POSITION_LEFT;
                        break;
                        case CONT_DPAD2_RIGHT:
                            hat2_state |= (uint32_t) HAT_POSITION_RIGHT;
                        break;
                        default:
                            break;
                        }
                    }
                }

                if(hat2_state != previous_hat2_state[i]) {
                    input_state->_handle_joystick_hat_motion(i, 0, (HatPosition) hat2_state);
                    previous_hat2_state[i] = (HatPosition) hat2_state;
                }

                previous_controller_button_state[i] = button_state;
            }
        }
    }

    /* FIXME: Support multiple keyboards */

    for(int8_t i = 0; i < 1; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_KEYBOARD);
        if(device) {
            auto state = (kbd_state_t*) maple_dev_status(device);

            auto get_modifiers = []() -> ModifierKeyState {
                ModifierKeyState mod_state;
                //FIXME:!
                return mod_state;
            };

            if(state) {
                std::array<uint8_t, 256> key_state;
                std::copy(state->matrix, state->matrix + 256, key_state.begin());

                for(uint32_t j = 0; j < 256; ++j) {
                    if(key_state[j] && !previous_key_state[j]) {
                        // Key down
                        input_state->_handle_key_down(
                            i, KeyboardCode(j)
                        );
                        on_key_down((KeyboardCode) j, get_modifiers());
                    }
                    if(!key_state[j] && previous_key_state[j]) {
                        // Key up
                        input_state->_handle_key_up(
                            i, KeyboardCode(j)
                        );

                        on_key_up((KeyboardCode) j, get_modifiers());
                    }
                }

                previous_key_state = key_state;
            }
        }
    }

}

std::shared_ptr<SoundDriver> KOSWindow::create_sound_driver(const std::string& from_config) {
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

void KOSWindow::initialize_input_controller(smlt::InputState &controller) {
    std::vector<JoystickDeviceInfo> joypads;

    auto mouse_dev = maple_enum_type(0, MAPLE_FUNC_MOUSE);
    if(mouse_dev) {
        MouseDeviceInfo mouse;
        mouse.id = 0;
        mouse.button_count = 3;
        mouse.axis_count = 2;
        controller._update_mouse_devices({mouse});
    }

    auto keyboard_dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
    if(keyboard_dev) {
        KeyboardDeviceInfo keyboard;
        keyboard.id = 0;
        controller._update_keyboard_devices({keyboard});
    }

    auto controller_count = 0u;
    for(int8_t i = 0; i < 4; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if(device) {
            JoystickDeviceInfo info;
            info.id = i;
            info.name = device->info.product_name;
            info.button_count = 5;
            info.axis_count = 4; //2 triggers, 2 for analog
            info.hat_count = 1; // 1 D-pad
            joypads.push_back(info);

            controller_count++;
        }
    }

    controller._update_joystick_devices(joypads);
}

void KOSWindow::render_screen(Screen* screen, const uint8_t* data) {
    thread::Lock<thread::Mutex> g(vmu_mutex_);

    auto it = vmu_lookup_.find(screen->name());

    if(it == vmu_lookup_.end()) {
        S_WARN("Tried to render to VMU that has been removed");
        return;
    }

    auto vmu = it->second;

    auto device = maple_enum_dev(vmu.first, vmu.second);
    if(device) {
        vmu_draw_lcd(device, (void*) data);
    }
}



}

