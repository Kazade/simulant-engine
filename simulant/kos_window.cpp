
#include <malloc.h>
#include <memory>
#include <vector>
#include "../deps/libgl/include/GL/gl.h"
#include "../deps/libgl/include/GL/glkos.h"

#include "input/input_state.h"
#include "kos_window.h"
#include "time_keeper.h"
#include "application.h"

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
    probe_vmus();
    return true;
}

bool KOSWindow::_init_renderer(Renderer* renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context

    S_DEBUG("Renderer initialized");

    return true;
}

void KOSWindow::destroy_window() {

}

static std::string vmu_port(int number) {
    /* Returns a, b, c, or d - depending if number is 0, 1, 2, or 3 */
    assert(number >= 0);
    assert(number < 4);
    char c(number + 97);
    return std::string(&c, 1);
}

void KOSWindow::probe_vmus() {
    const static int MAX_VMUS = 8;  // Four ports, two slots in each

    thread::Lock<thread::Mutex> g(vmu_mutex_);

    std::unordered_map<std::string, std::pair<int, int>> new_lookup;

    for(auto i = 0; i < MAX_VMUS; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_LCD);
        if(device && device->valid) {
            std::string identifier = _F("{0}{1}").format(
                vmu_port(device->port),
                device->unit
            );
            new_lookup[identifier] = std::make_pair(device->port, device->unit);
        }
    }

    for(auto& p: vmu_lookup_) {
        if(new_lookup.find(p.first) == new_lookup.end()) {
            /* Delete the screen */
            _destroy_screen(p.first);
        }
    }

    for(auto& p: new_lookup) {
        if(vmu_lookup_.find(p.first) == vmu_lookup_.end()) {
            /* Create a screen */
            _create_screen(p.first, 48, 32, SCREEN_FORMAT_G1, 5);
            S_DEBUG("Creating screen for VMU: {0}", p.first);
        }
    }

    vmu_lookup_ = new_lookup;
}

static constexpr JoystickButton dc_button_to_simulant_button(uint16_t dc_button) {
    return (dc_button == CONT_A) ? JOYSTICK_BUTTON_A :
           (dc_button == CONT_B) ? JOYSTICK_BUTTON_B :
           (dc_button == CONT_C) ? JOYSTICK_BUTTON_LEFT_SHOULDER :
           (dc_button == CONT_X) ? JOYSTICK_BUTTON_X :
           (dc_button == CONT_Y) ? JOYSTICK_BUTTON_Y :
           (dc_button == CONT_Z) ? JOYSTICK_BUTTON_RIGHT_SHOULDER :
           (dc_button == CONT_START) ? JOYSTICK_BUTTON_START :
           (dc_button == CONT_DPAD_UP) ? JOYSTICK_BUTTON_DPAD_UP :
           (dc_button == CONT_DPAD_DOWN) ? JOYSTICK_BUTTON_DPAD_DOWN :
           (dc_button == CONT_DPAD_LEFT) ? JOYSTICK_BUTTON_DPAD_LEFT :
           (dc_button == CONT_DPAD_RIGHT) ? JOYSTICK_BUTTON_DPAD_RIGHT :
           (dc_button == CONT_DPAD2_UP) ? JOYSTICK_BUTTON_DPAD2_UP :
           (dc_button == CONT_DPAD2_DOWN) ? JOYSTICK_BUTTON_DPAD2_DOWN :
           (dc_button == CONT_DPAD2_LEFT) ? JOYSTICK_BUTTON_DPAD2_LEFT :
           (dc_button == CONT_DPAD2_RIGHT) ? JOYSTICK_BUTTON_DPAD2_RIGHT :
           JOYSTICK_BUTTON_INVALID;
}

void KOSWindow::check_events() {
    float dt = application->time_keeper->delta_time();

    /* Regularly recheck the controller state */
    time_since_last_controller_update_ += application->time_keeper->delta_time();
    if(time_since_last_controller_update_ > 1.0f) {
        time_since_last_controller_update_ = 0.0f;

        // Rescan for devices in case a controller has been added or removed
        //initialize_input_controller(*this->_input_state());
        probe_vmus();
    }

    const int8_t MAX_CONTROLLERS = 4;
    const static uint16_t CONTROLLER_BUTTONS [] = {
        CONT_A, CONT_B, CONT_C, CONT_D, CONT_X, CONT_Y, CONT_Z, CONT_START,
        CONT_DPAD_UP, CONT_DPAD_DOWN, CONT_DPAD_LEFT, CONT_DPAD_RIGHT,
        CONT_DPAD2_UP, CONT_DPAD2_DOWN, CONT_DPAD2_LEFT, CONT_DPAD2_RIGHT
    };

    static uint32_t previous_controller_button_state[MAX_CONTROLLERS] = {0};
    static uint8_t previous_key_state[MAX_KEYBOARD_CODES] = {0}; // value-initialize to zero

    auto& previous = previous_controller_state_;

    /* Check controller states */
    for(int8_t i = 0; i < MAX_CONTROLLERS; ++i) {
        auto id = GameControllerID(i);

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

                auto handle_joystick_axis = [this](int prev, int current, GameControllerID controller, smlt::JoystickAxis target, float range=127.0f) -> int16_t {
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

                previous[i].joyx = handle_joystick_axis(previous[i].joyx, joyx_state, id, JOYSTICK_AXIS_X);
                previous[i].joyy = handle_joystick_axis(previous[i].joyy, joyy_state, id, JOYSTICK_AXIS_Y);
                previous[i].joyx2 = handle_joystick_axis(previous[i].joyx2, joyx2_state, id, JOYSTICK_AXIS_2);
                previous[i].joyy2 = handle_joystick_axis(previous[i].joyy2, joyy2_state, id, JOYSTICK_AXIS_3);
                previous[i].ltrig = handle_joystick_axis(previous[i].ltrig, ltrig_state, id, JOYSTICK_AXIS_4, 255.0f);
                previous[i].rtrig = handle_joystick_axis(previous[i].rtrig, rtrig_state, id, JOYSTICK_AXIS_5, 255.0f);

                /* Reduce the current rumble remaining if necessary */
                previous[i].current_rumble_remaining_ = Seconds(
                    std::max(previous[i].current_rumble_remaining_.to_float() - dt, 0.0f)
                );

                // Check the current button state against the previous one
                // and update the input controller appropriately
                for(auto button: CONTROLLER_BUTTONS) {
                    bool prev = prev_state & button;
                    bool curr = button_state & button;

                    if(!prev && curr) {
                        // Button down
                        input_state->_handle_joystick_button_down(
                            id, dc_button_to_simulant_button(button)
                        );

                        on_game_controller_button_down(GameControllerIndex(i), dc_button_to_simulant_button(button));

                    } else if(prev && !curr) {
                        // Button up
                        input_state->_handle_joystick_button_up(
                            id, dc_button_to_simulant_button(button)
                        );

                        on_game_controller_button_up(GameControllerIndex(i), dc_button_to_simulant_button(button));
                    }
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

            auto get_modifiers = [&state]() -> ModifierKeyState {
                ModifierKeyState mod_state;
                mod_state.lctrl = state->shift_keys & KBD_MOD_LCTRL;
                mod_state.rctrl = state->shift_keys & KBD_MOD_RCTRL;
                mod_state.rshift = state->shift_keys & KBD_MOD_RSHIFT;
                mod_state.lshift = state->shift_keys & KBD_MOD_LSHIFT;
                mod_state.ralt = state->shift_keys & KBD_MOD_RALT;
                mod_state.lalt = state->shift_keys & KBD_MOD_LALT;

                //FIXME:!
                return mod_state;
            };

            if(state) {
                const uint8_t* key_state = state->matrix;

                for(uint32_t j = 0; j < MAX_KEYBOARD_CODES; ++j) {
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

                std::copy(key_state, key_state + MAX_KEYBOARD_CODES, previous_key_state);
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
    std::vector<GameControllerInfo> joypads;

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

    const int MAX_DEVICES_PER_PORT = 6;

    S_DEBUG("{0} devices connected", maple_enum_count());

    auto controller_count = 0u;
    for(int8_t i = 0; i < 4; ++i) {
        auto device = maple_enum_dev(i, 0);
        if(device && (device->info.functions & MAPLE_FUNC_CONTROLLER)) {
            S_DEBUG("Found controller at port {0} unit {1}", device->port, device->unit);
            GameControllerInfo info;
            info.id = GameControllerID(device->port);
            std::strncpy(info.name, device->info.product_name, sizeof(info.name));
            info.button_count = 5;
            info.axis_count = 4; //2 triggers, 2 for analog
            info.hat_count = 1; // 1 D-pad
            info.has_rumble = false;

            for(int j = 1; j < MAX_DEVICES_PER_PORT; ++j) {
                auto purupuru = maple_enum_dev(device->port, j);
                if(purupuru && (purupuru->info.functions & MAPLE_FUNC_PURUPURU)) {
                    info.has_rumble = true;
                    /* Store the unit in the platform data */
                    info.platform_data.b[0] = purupuru->unit;
                    S_DEBUG("Found rumble at port {0} unit {1}", purupuru->port, purupuru->unit);
                    break;
                }
            }

            joypads.push_back(info);

            controller_count++;
        }
    }

    S_DEBUG("Calling controller update with {0} controllers", controller_count);
    controller._update_game_controllers(joypads);
}

void KOSWindow::render_screen(Screen* screen, const uint8_t* data, int row_stride) {
    thread::Lock<thread::Mutex> g(vmu_mutex_);

    auto it = vmu_lookup_.find(screen->name());

    if(it == vmu_lookup_.end()) {
        S_WARN("Tried to render to VMU that has been removed");
        return;
    }

    auto vmu = it->second;

    std::vector<uint8_t> rw_data(data, data + 192);

    auto device = maple_enum_dev(vmu.first, vmu.second);
    if(device) {
        int err = vmu_draw_lcd(device, (uint8_t*) &rw_data[0]);
        if(err < 0) {
            S_ERROR("There was an error updating the VMU LCD: {0}", err);
        }
    }
}

void KOSWindow::game_controller_start_rumble(GameController* controller, RangeValue<0, 1> low_rumble, RangeValue<0, 1> high_rumble, const smlt::Seconds& duration) {
    _S_UNUSED(high_rumble);

    /* Don't resend rumble if we're still rumbling (matches SDL) */
    auto& state = previous_controller_state_[controller->id().to_int8_t()]; //FIXME: id vs index, this is likely bad
    if(state.current_rumble_remaining_.to_float() > 0.0f) {
        return;
    }

    const float M = 4;

    if(controller && controller->has_rumble_effect()) {
        const uint8_t* pdata = controller->platform_data();
        auto purupuru_unit = pdata[0];

        int intensity = smlt::clamp(low_rumble * 7, 0, 7);
        uint8_t length = std::max(std::min(duration.to_float() * M, 255.0f), 4.0f);

        auto device = maple_enum_dev(controller->id().to_int8_t(), purupuru_unit);
        if(device && (device->info.functions & MAPLE_FUNC_PURUPURU)) {
            purupuru_effect_t effect;
            effect.duration = length;
            effect.effect1 = PURUPURU_EFFECT1_INTENSITY(intensity) | PURUPURU_EFFECT1_PULSE;
            effect.special = PURUPURU_SPECIAL_MOTOR1;
            int retries = 5;
            while(--retries && purupuru_rumble(device, &effect) == MAPLE_EAGAIN) {}
            if(!retries) {
                S_WARN("PURUPURU FAILED: {0} {1} -> {2}", device->port, device->unit, intensity);
            } else {
                state.current_rumble_remaining_ = duration;
                S_DEBUG("PURUPURU: {0} {1} -> {2}", device->port, device->unit, intensity);
            }
        } else {
            S_WARN("Failed to start rumble - couldn't find PURUPURU");
        }
    }
}

void KOSWindow::game_controller_stop_rumble(GameController *controller) {
    if(controller && controller->has_rumble_effect()) {
        auto& state = previous_controller_state_[controller->id().to_int8_t()]; //FIXME: id vs index, this is likely bad

        const uint8_t* pdata = controller->platform_data();
        auto purupuru_unit = pdata[0];
        auto device = maple_enum_dev(controller->id().to_int8_t(), purupuru_unit);

        purupuru_effect_t effect;
        effect.duration = 0x00;
        effect.effect2 = 0x00;
        effect.effect1 = 0x00;
        effect.special = PURUPURU_SPECIAL_MOTOR1;
        purupuru_rumble(device, &effect);
        S_DEBUG("Stopped PURPURU");
        state.current_rumble_remaining_ = Seconds(0.0f);
    }
}

}

