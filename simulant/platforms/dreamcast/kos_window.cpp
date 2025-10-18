
#include <malloc.h>
#include <memory>
#include <vector>
#include "../../../deps/libgl/include/GL/gl.h"
#include "../../../deps/libgl/include/GL/glkos.h"

#include "../../input/input_state.h"
#include "kos_window.h"
#include "../../time_keeper.h"
#include "../../application.h"

#include "../../sound/drivers/openal_sound_driver.h"
#include "../../sound/drivers/null_sound_driver.h"

#include "../../renderers/renderer_config.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_DEPTH 32

namespace smlt {

/* pimpl to avoid exposing implementation */
struct KOSWindowPrivate {
    /* VMU LCD update queue (OK it's not really a queue...)
     *
     * Basically updating a VMU screen is a blocking operation, and if we do that in the main
     * thread we're in for bad times.
     *
     * So instead we use a long-running thread which literally sits waiting for something to appear
     * in this map, and then uploads the data to the appropriate VMU, then removes it from the map.
     *
     * The key of the map is (port << 4 | unit) where port and unit are the things passed to
     * maple_enum_dev to identify the target VMU.
     */
    std::unordered_map<uint8_t, std::vector<uint8_t>> vmu_lcd_update_queue_;
    smlt::thread::Atomic<bool> lcd_updates_running_ = {true};
    smlt::thread::Future<void> update_future_;

    /* Name, to port/unit combo. This only includes VMUs we've seen during the last probe */
    thread::Mutex vmu_mutex_;
    std::unordered_map<std::string, std::pair<int, int>> vmu_lookup_;
};

static void vmu_lcd_update_thread(KOSWindowPrivate* self) {
    S_DEBUG("Starting LCD update thread");

    while(self->lcd_updates_running_) {
        /* Check for updates */
        do {
            smlt::thread::sleep(10);
            {
                thread::Lock<thread::Mutex> g(self->vmu_mutex_);
                if(!self->vmu_lcd_update_queue_.empty()) {
                    break;
                }
            }
        } while(self->lcd_updates_running_);

        /* Did we break the above loop because we're quitting? */
        if(!self->lcd_updates_running_) {
            break;
        }

        /* Update the vmur devices */
        thread::Lock<thread::Mutex> g(self->vmu_mutex_);
        for(auto& p: self->vmu_lcd_update_queue_) {
            auto unit = p.first & 0xF;
            auto port = p.first >> 4;

            auto device = maple_enum_dev(port, unit);
            if(device) {
                int err = vmu_draw_lcd(device, (uint8_t*) &p.second[0]);
                if(err < 0) {
                    S_ERROR("There was an error updating the VMU LCD: {0}", err);
                }
            }
        }
        self->vmu_lcd_update_queue_.clear();
    }

    S_DEBUG("Ended LCD update thread");
}

KOSWindow::KOSWindow():
    private_(new KOSWindowPrivate()) {

    /* Run the lcd update thread async */
    private_->update_future_ = smlt::thread::async(
        &vmu_lcd_update_thread, private_.get()
    );
}

KOSWindow::~KOSWindow() {
    if(private_) {
        private_->lcd_updates_running_ = false;
        private_->update_future_.wait();
    }
}

void KOSWindow::do_swap_buffers() {
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
    /* Stop the update thread */
    private_->lcd_updates_running_ = false;
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

    thread::Lock<thread::Mutex> g(private_->vmu_mutex_);

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

    for(auto& p: private_->vmu_lookup_) {
        if(new_lookup.find(p.first) == new_lookup.end()) {
            /* Delete the screen */
            _destroy_screen(p.first);
        }
    }

    for(auto& p: new_lookup) {
        if(private_->vmu_lookup_.find(p.first) == private_->vmu_lookup_.end()) {
            /* Create a screen */
            _create_screen(p.first, 48, 32, SCREEN_FORMAT_G1, 5);
            S_DEBUG("Creating screen for VMU: {0}", p.first);
        }
    }

    private_->vmu_lookup_ = new_lookup;
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

static KeyboardCode scancode_to_keyboard_code(int32_t code) {
    switch(code) {
        case KBD_KEY_A:
            return KEYBOARD_CODE_A;
        case KBD_KEY_B:
            return KEYBOARD_CODE_B;
        case KBD_KEY_C:
            return KEYBOARD_CODE_C;
        case KBD_KEY_D:
            return KEYBOARD_CODE_D;
        case KBD_KEY_E:
            return KEYBOARD_CODE_E;
        case KBD_KEY_F:
            return KEYBOARD_CODE_F;
        case KBD_KEY_G:
            return KEYBOARD_CODE_G;
        case KBD_KEY_H:
            return KEYBOARD_CODE_H;
        case KBD_KEY_I:
            return KEYBOARD_CODE_I;
        case KBD_KEY_J:
            return KEYBOARD_CODE_J;
        case KBD_KEY_K:
            return KEYBOARD_CODE_K;
        case KBD_KEY_L:
            return KEYBOARD_CODE_L;
        case KBD_KEY_M:
            return KEYBOARD_CODE_M;
        case KBD_KEY_N:
            return KEYBOARD_CODE_N;
        case KBD_KEY_O:
            return KEYBOARD_CODE_O;
        case KBD_KEY_P:
            return KEYBOARD_CODE_P;
        case KBD_KEY_Q:
            return KEYBOARD_CODE_Q;
        case KBD_KEY_R:
            return KEYBOARD_CODE_R;
        case KBD_KEY_S:
            return KEYBOARD_CODE_S;
        case KBD_KEY_T:
            return KEYBOARD_CODE_T;
        case KBD_KEY_U:
            return KEYBOARD_CODE_U;
        case KBD_KEY_V:
            return KEYBOARD_CODE_V;
        case KBD_KEY_W:
            return KEYBOARD_CODE_W;
        case KBD_KEY_X:
            return KEYBOARD_CODE_X;
        case KBD_KEY_Y:
            return KEYBOARD_CODE_Y;
        case KBD_KEY_Z:
            return KEYBOARD_CODE_Z;
        case KBD_KEY_1:
            return KEYBOARD_CODE_1;
        case KBD_KEY_2:
            return KEYBOARD_CODE_2;
        case KBD_KEY_3:
            return KEYBOARD_CODE_3;
        case KBD_KEY_4:
            return KEYBOARD_CODE_4;
        case KBD_KEY_5:
            return KEYBOARD_CODE_5;
        case KBD_KEY_6:
            return KEYBOARD_CODE_6;
        case KBD_KEY_7:
            return KEYBOARD_CODE_7;
        case KBD_KEY_8:
            return KEYBOARD_CODE_8;
        case KBD_KEY_9:
            return KEYBOARD_CODE_9;
        case KBD_KEY_0:
            return KEYBOARD_CODE_0;
        case KBD_KEY_ENTER:
            return KEYBOARD_CODE_RETURN;
        case KBD_KEY_ESCAPE:
            return KEYBOARD_CODE_ESCAPE;
        case KBD_KEY_BACKSPACE:
            return KEYBOARD_CODE_BACKSPACE;
        case KBD_KEY_TAB:
            return KEYBOARD_CODE_TAB;
        case KBD_KEY_SPACE:
            return KEYBOARD_CODE_SPACE;
        case KBD_KEY_MINUS:
            return KEYBOARD_CODE_MINUS;
        case KBD_KEY_PLUS:
            return KEYBOARD_CODE_EQUALS;
        case KBD_KEY_LBRACKET:
            return KEYBOARD_CODE_LEFTBRACKET;
        case KBD_KEY_RBRACKET:
            return KEYBOARD_CODE_RIGHTBRACKET;
        case KBD_KEY_BACKSLASH:
            return KEYBOARD_CODE_BACKSLASH;
        case KBD_KEY_SEMICOLON:
            return KEYBOARD_CODE_SEMICOLON;
        case KBD_KEY_QUOTE:
            return KEYBOARD_CODE_APOSTROPHE;
        case KBD_KEY_TILDE:
            return KEYBOARD_CODE_GRAVE;
        case KBD_KEY_COMMA:
            return KEYBOARD_CODE_COMMA;
        case KBD_KEY_PERIOD:
            return KEYBOARD_CODE_PERIOD;
        case KBD_KEY_SLASH:
            return KEYBOARD_CODE_SLASH;
        case KBD_KEY_CAPSLOCK:
            return KEYBOARD_CODE_CAPSLOCK;
        case KBD_KEY_F1:
            return KEYBOARD_CODE_F1;
        case KBD_KEY_F2:
            return KEYBOARD_CODE_F2;
        case KBD_KEY_F3:
            return KEYBOARD_CODE_F3;
        case KBD_KEY_F4:
            return KEYBOARD_CODE_F4;
        case KBD_KEY_F5:
            return KEYBOARD_CODE_F5;
        case KBD_KEY_F6:
            return KEYBOARD_CODE_F6;
        case KBD_KEY_F7:
            return KEYBOARD_CODE_F7;
        case KBD_KEY_F8:
            return KEYBOARD_CODE_F8;
        case KBD_KEY_F9:
            return KEYBOARD_CODE_F9;
        case KBD_KEY_F10:
            return KEYBOARD_CODE_F10;
        case KBD_KEY_F11:
            return KEYBOARD_CODE_F11;
        case KBD_KEY_F12:
            return KEYBOARD_CODE_F12;
        case KBD_KEY_PRINT:
            return KEYBOARD_CODE_PRINTSCREEN;
        case KBD_KEY_SCRLOCK:
            return KEYBOARD_CODE_SCROLLLOCK;
        case KBD_KEY_PAUSE:
            return KEYBOARD_CODE_PAUSE;
        case KBD_KEY_INSERT:
            return KEYBOARD_CODE_INSERT;
        case KBD_KEY_HOME:
            return KEYBOARD_CODE_HOME;
        case KBD_KEY_PGUP:
            return KEYBOARD_CODE_PAGEUP;
        case KBD_KEY_DEL:
            return KEYBOARD_CODE_DELETE;
        case KBD_KEY_END:
            return KEYBOARD_CODE_END;
        case KBD_KEY_PGDOWN:
            return KEYBOARD_CODE_PAGEDOWN;
        case KBD_KEY_RIGHT:
            return KEYBOARD_CODE_RIGHT;
        case KBD_KEY_LEFT:
            return KEYBOARD_CODE_LEFT;
        case KBD_KEY_DOWN:
            return KEYBOARD_CODE_DOWN;
        case KBD_KEY_UP:
            return KEYBOARD_CODE_UP;
        case KBD_KEY_PAD_NUMLOCK:
            return KEYBOARD_CODE_NUMLOCKCLEAR;
        case KBD_KEY_PAD_DIVIDE:
            return KEYBOARD_CODE_KP_DIVIDE;
        case KBD_KEY_PAD_MULTIPLY:
            return KEYBOARD_CODE_KP_MULTIPLY;
        case KBD_KEY_PAD_MINUS:
            return KEYBOARD_CODE_KP_MINUS;
        case KBD_KEY_PAD_PLUS:
            return KEYBOARD_CODE_KP_PLUS;
        case KBD_KEY_PAD_ENTER:
            return KEYBOARD_CODE_KP_ENTER;
        case KBD_KEY_PAD_1:
            return KEYBOARD_CODE_KP_1;
        case KBD_KEY_PAD_2:
            return KEYBOARD_CODE_KP_2;
        case KBD_KEY_PAD_3:
            return KEYBOARD_CODE_KP_3;
        case KBD_KEY_PAD_4:
            return KEYBOARD_CODE_KP_4;
        case KBD_KEY_PAD_5:
            return KEYBOARD_CODE_KP_5;
        case KBD_KEY_PAD_6:
            return KEYBOARD_CODE_KP_6;
        case KBD_KEY_PAD_7:
            return KEYBOARD_CODE_KP_7;
        case KBD_KEY_PAD_8:
            return KEYBOARD_CODE_KP_8;
        case KBD_KEY_PAD_9:
            return KEYBOARD_CODE_KP_9;
        case KBD_KEY_PAD_0:
            return KEYBOARD_CODE_KP_0;
        case KBD_KEY_PAD_PERIOD:
            return KEYBOARD_CODE_KP_PERIOD;
        default:
            return KEYBOARD_CODE_NONE;
    }
}

void KOSWindow::check_events() {
    float dt = app->time_keeper->delta_time();

    /* Regularly recheck the controller state */
    time_since_last_controller_update_ += dt;
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

    auto& previous = previous_controller_state_;

    /* Check controller states */
    for(int8_t i = 0; i < MAX_CONTROLLERS; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if(device) {
            auto id = GameControllerID(device->port);
            auto state = (cont_state_t*) maple_dev_status(device);
            if(state) {
                auto port = device->port;
                auto button_state = state->buttons;
                auto prev_state = previous_controller_button_state_[port];
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

                previous[port].joyx = handle_joystick_axis(previous[port].joyx, joyx_state, id, JOYSTICK_AXIS_X);
                previous[port].joyy = handle_joystick_axis(previous[port].joyy, joyy_state, id, JOYSTICK_AXIS_Y);
                previous[port].joyx2 = handle_joystick_axis(previous[port].joyx2, joyx2_state, id, JOYSTICK_AXIS_2);
                previous[port].joyy2 = handle_joystick_axis(previous[port].joyy2, joyy2_state, id, JOYSTICK_AXIS_3);
                previous[port].ltrig = handle_joystick_axis(previous[port].ltrig, ltrig_state, id, JOYSTICK_AXIS_4, 255.0f);
                previous[port].rtrig = handle_joystick_axis(previous[port].rtrig, rtrig_state, id, JOYSTICK_AXIS_5, 255.0f);

                /* Reduce the current rumble remaining if necessary */
                previous[port].current_rumble_remaining_ = Seconds(
                    std::max(previous[port].current_rumble_remaining_.to_float() - dt, 0.0f)
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

                previous_controller_button_state_[port] = button_state;
            } else {
                previous_controller_state_[device->port] = ControllerState();
                previous_controller_button_state_[device->port] = 0;
            }
        }
    }

    /* FIXME: Support multiple keyboards */

    for(int8_t i = 0; i < 1; ++i) {
        auto device = maple_enum_type(i, MAPLE_FUNC_KEYBOARD);
        if(device) {
            /* Make sure we update that there's a keyboard available. We
             * only support 1 for now! */

            KeyboardDeviceInfo keyboard;
            keyboard.id = 0;
            input_state->_update_keyboard_devices({keyboard});

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
                    if(key_state[j] && !previous_key_state_[j]) {
                        // Key down
                        input_state->_handle_key_down(
                            KeyboardID(i), scancode_to_keyboard_code(j));
                        on_key_down(scancode_to_keyboard_code(j),
                                    get_modifiers());
                    }
                    if(!key_state[j] && previous_key_state_[j]) {
                        // Key up
                        input_state->_handle_key_up(
                            KeyboardID(i), scancode_to_keyboard_code(j));

                        on_key_up(scancode_to_keyboard_code(j),
                                  get_modifiers());
                    }
                }

                std::copy(key_state, key_state + MAX_KEYBOARD_CODES, previous_key_state_);
            }
        } else {
            input_state->_update_keyboard_devices({});
            std::fill(previous_key_state_, previous_key_state_ + MAX_KEYBOARD_CODES, 0);
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

#define PLATFORM_DATA_PORT 0
#define PLATFORM_DATA_PURUPURU 1

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

            info.platform_data.b[PLATFORM_DATA_PORT] = device->port;

            for(int j = 1; j < MAX_DEVICES_PER_PORT; ++j) {
                auto purupuru = maple_enum_dev(device->port, j);
                if(purupuru && (purupuru->info.functions & MAPLE_FUNC_PURUPURU)) {
                    info.has_rumble = true;
                    /* Store the unit in the platform data */
                    info.platform_data.b[PLATFORM_DATA_PURUPURU] = j;
                    S_DEBUG("Found rumble at port {0} unit {1}", purupuru->port, j);
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
    thread::Lock<thread::Mutex> g(private_->vmu_mutex_);

    auto it = private_->vmu_lookup_.find(screen->name());

    if(it == private_->vmu_lookup_.end()) {
        S_WARN("Tried to render to VMU that has been removed");
        return;
    }

    auto vmu = it->second;

    std::vector<uint8_t> rw_data(data, data + 192);

    uint8_t id = vmu.first << 4 | (vmu.second & 0xF);
    private_->vmu_lcd_update_queue_.insert(std::make_pair(id, std::move(rw_data)));
    S_VERBOSE("Pushed VMU update to queue for device {0}", id);
}

void KOSWindow::game_controller_start_rumble(GameController* controller, NormalizedFloat low_rumble, NormalizedFloat high_rumble, const smlt::Seconds& duration) {
    const uint8_t* pdata = controller->platform_data();
    auto port = pdata[PLATFORM_DATA_PORT];

    /* Don't resend rumble if we're still rumbling (matches SDL) */
    auto& state = previous_controller_state_[port];
    if(state.current_rumble_remaining_.to_float() > 0.0f) {
        return;
    }

    const float M = 4;

    if(controller && controller->has_rumble_effect()) {
        /* DC doesn't generally have two motors, so we average the input of the two values */
        int intensity = smlt::clamp((((float) low_rumble + (float) high_rumble) * 0.5f) * 7, 0, 7);
        uint8_t length = std::max(std::min(duration.to_float() * M, 255.0f), 4.0f);
        auto purupuru_unit = pdata[PLATFORM_DATA_PURUPURU];

        auto device = maple_enum_dev(port, purupuru_unit);
        if(device && (device->info.functions & MAPLE_FUNC_PURUPURU)) {
            purupuru_effect_t effect;
            effect.duration = length;
            effect.effect1 = PURUPURU_EFFECT1_INTENSITY(intensity) | PURUPURU_EFFECT1_PULSE;
            effect.effect2 = PURUPURU_EFFECT2_LINTENSITY(7) | PURUPURU_EFFECT2_PULSE;
            effect.special = PURUPURU_SPECIAL_MOTOR1;
            auto ret = purupuru_rumble(device, &effect);

            if(ret != MAPLE_EOK) {
                state.current_rumble_remaining_ = Seconds(0.0f);
                S_DEBUG("PURUPURU FAILED: {0} {1} -> {2} for {3}", device->port, device->unit, intensity, +length);
            } else {
                state.current_rumble_remaining_ = duration;
                S_DEBUG("PURUPURU: {0} {1} -> {2} for {3}", device->port, device->unit, intensity, +length);
            }
        } else {
            S_WARN("Failed to start rumble - couldn't find PURUPURU");
        }
    }
}

void KOSWindow::game_controller_stop_rumble(GameController *controller) {
    if(controller && controller->has_rumble_effect()) {
        const uint8_t* pdata = controller->platform_data();
        auto port = pdata[PLATFORM_DATA_PORT];
        auto& state = previous_controller_state_[port];

        /* We're not rumbling, don't send */
        if(state.current_rumble_remaining_.to_float() <= 0.0f) {
            return;
        }

        auto purupuru_unit = pdata[PLATFORM_DATA_PURUPURU];

        auto device = maple_enum_dev(port, purupuru_unit);
        if(device && (device->info.functions & MAPLE_FUNC_PURUPURU)) {
            purupuru_effect_t effect;
            effect.duration = 0x00;
            effect.effect2 = 0x00;
            effect.effect1 = 0x00;
            effect.special = PURUPURU_SPECIAL_MOTOR1;
            auto ret = purupuru_rumble(device, &effect);

            if(ret != MAPLE_EOK) {
                S_DEBUG("PURUPURU STOP FAILED: {0} {1} ({2})", device->port, device->unit, ret);
            } else {
                state.current_rumble_remaining_ = Seconds(0.0f);
                S_DEBUG("Stopped PURUPURU");
            }
        }
    }
}

} // namespace smlt
