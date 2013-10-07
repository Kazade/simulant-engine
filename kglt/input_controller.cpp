#include "kazbase/list_utils.h"
#include "kazbase/unicode.h"
#include "kazbase/logging.h"
#include "input_controller.h"

namespace kglt {

InputConnection::InputConnection(InputConnectionID id, Device &device):
    generic::Identifiable<InputConnectionID>(id),
    device_(device) {

}

InputConnection Device::new_input_connection() {
    static uint32_t idx = 0;
    return InputConnection(InputConnectionID(idx), *this);
}

InputConnection Keyboard::key_pressed_connect(GlobalKeyCallback callback) {
    InputConnection c = new_input_connection();
    global_key_press_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_pressed_connect(SDL_Scancode code, KeyCallback callback) {
    InputConnection c = new_input_connection();
    key_press_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_while_down_connect(SDL_Scancode code, KeyDownCallback callback) {
    InputConnection c = new_input_connection();
    key_while_down_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_released_connect(SDL_Scancode code, KeyCallback callback) {
    InputConnection c = new_input_connection();
    key_release_signals_[code][c] = callback;
    return c;
}

Keyboard::Keyboard() {
    current_mods_ = KMOD_NONE;
}

void Keyboard::_handle_keydown_event(SDL_Keysym key) {
    //Store modifiers
    switch(key.scancode) {
    case SDL_SCANCODE_NUMLOCKCLEAR:
        current_mods_ |= KMOD_NUM;
    break;
    case SDL_SCANCODE_CAPSLOCK:
        current_mods_ |= KMOD_CAPS;
    break;
    case SDL_SCANCODE_LCTRL: {
        current_mods_ |= KMOD_LCTRL;
        current_mods_ |= KMOD_CTRL;
    } break;
    case SDL_SCANCODE_RCTRL: {
        current_mods_ |= KMOD_RCTRL;
        current_mods_ |= KMOD_CTRL;
    } break;
    case SDL_SCANCODE_LSHIFT: {
        current_mods_ |= KMOD_LSHIFT;
        current_mods_ |= KMOD_SHIFT;
    } break;
    case SDL_SCANCODE_RSHIFT: {
        current_mods_ |= KMOD_RSHIFT;
        current_mods_ |= KMOD_SHIFT;
    } break;
    case SDL_SCANCODE_LALT: {
        current_mods_ |= KMOD_LALT;
        current_mods_ |= KMOD_ALT;
    } break;
    case SDL_SCANCODE_RALT: {
        current_mods_ |= KMOD_RALT;
        current_mods_ |= KMOD_ALT;
    } break;
    default:
        break;
    }

    bool propagation_stopped = false;

    //First trigger all global handlers
    for(GlobalKeySignalEntry entry: global_key_press_signals_) {
        if(entry.second(key)) {
            propagation_stopped = true;
        }
    }

    //If a global handler returned true, we don't trigger any more
    //signals
    if(!propagation_stopped) {
        if(container::contains(key_press_signals_, key.scancode)) {
            for(KeySignalEntry entry: key_press_signals_[key.scancode]) {
                entry.second(key);
            }
        }

        //This is inside the IF because, otherwise while_key_pressed events will
        //trigger and we don't want that if the key down was handled... feels dirty
        state_[key.scancode] = true;
    }
}

bool modifier_is_set(uint32_t state, SDL_Keymod modifier) {
    return (state & modifier) == modifier;
}

void Keyboard::_handle_keyup_event(SDL_Keysym key) {

    //Unset modifiers
    switch(key.scancode) {
    case SDL_SCANCODE_NUMLOCKCLEAR:
        current_mods_ &= ~KMOD_NUM;
    break;
    case SDL_SCANCODE_CAPSLOCK:
        current_mods_ &= ~KMOD_CAPS;
    break;
    case SDL_SCANCODE_LCTRL: {
        current_mods_ &= ~KMOD_LCTRL;
        //If the RCTRL isn't set, remove the CTRL modifier
        if(!modifier_is_set(current_mods_, KMOD_RCTRL)) {
            current_mods_ &= ~KMOD_CTRL;
        }
    } break;
    case SDL_SCANCODE_RCTRL: {
        current_mods_ &= ~KMOD_RCTRL;
        //If the LCTRL isn't set, remove the CTRL modifier
        if(!modifier_is_set(current_mods_, KMOD_LCTRL)) {
            current_mods_ &= ~KMOD_CTRL;
        }
    } break;
    case SDL_SCANCODE_LSHIFT: {
        current_mods_ &= ~KMOD_LSHIFT;
        if(!modifier_is_set(current_mods_, KMOD_RSHIFT)) {
            current_mods_ &= ~KMOD_SHIFT;
        }
    } break;
    case SDL_SCANCODE_RSHIFT: {
        current_mods_ &= ~KMOD_RSHIFT;
        if(!modifier_is_set(current_mods_, KMOD_LSHIFT)) {
            current_mods_ &= ~KMOD_SHIFT;
        }
    } break;
    case SDL_SCANCODE_LALT: {
        current_mods_ &= ~KMOD_LALT;
        if(!modifier_is_set(current_mods_, KMOD_RALT)) {
            current_mods_ &= ~KMOD_ALT;
        }
    } break;
    case SDL_SCANCODE_RALT: {
        current_mods_ &= ~KMOD_RALT;
        if(!modifier_is_set(current_mods_, KMOD_LALT)) {
            current_mods_ &= ~KMOD_ALT;
        }
    } break;
    default:
        break;
    }

    if(container::contains(key_release_signals_, key.scancode)) {
        for(KeySignalEntry entry: key_release_signals_[key.scancode]) {
            entry.second(key);
        }
    }
    state_[key.scancode] = false;

}

void Keyboard::_update(double dt) {
    for(auto p: key_while_down_signals_) {
        if(key_state(p.first)) {
            for(std::pair<InputConnection, KeyDownCallback> p2: p.second) {
                p2.second(p.first, current_mods_, dt);
            }
        }
    }
}

void Keyboard::_disconnect(const InputConnection &connection) {
    for(auto p: key_press_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    for(auto p: key_while_down_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    for(auto p: key_release_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }
}

Joypad::Joypad():
    jitter_value_(3) {

}

InputConnection Joypad::axis_changed_connect(Axis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_changed_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_nonzero_connect(Axis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_nonzero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_below_zero_connect(Axis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_below_zero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_above_zero_connect(Axis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_above_zero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::button_pressed_connect(Button button, JoypadButtonCallback callback) {
    InputConnection c = new_input_connection();
    button_pressed_signals_[button][c] = callback;
    return c;
}

InputConnection Joypad::button_released_connect(Button button, JoypadButtonCallback callback) {
    InputConnection c = new_input_connection();
    button_released_signals_[button][c] = callback;
    return c;
}

InputConnection Joypad::hat_changed_connect(Hat hat, JoypadHatCallback callback) {
    InputConnection c = new_input_connection();
    hat_changed_signals_[hat][c] = callback;
    return c;
}

void Joypad::_handle_button_down_event(Button button) {
    if(container::contains(button_pressed_signals_, button)) {
        for(ButtonSignalEntry entry: button_pressed_signals_[button]) {
            entry.second(button);
        }
    }
}

void Joypad::_handle_button_up_event(Button button) {
    if(container::contains(button_released_signals_, button)) {
        for(ButtonSignalEntry entry: button_released_signals_[button]) {
            entry.second(button);
        }
    }
}

void Joypad::_handle_axis_changed_event(Axis axis, int32_t value) {
    if(container::contains(axis_changed_signals_, axis)) {
        for(AxisSignalEntry entry: axis_changed_signals_[axis]) {
            entry.second(float(value) / float(32768), axis);
        }
    }

    axis_state_[axis] = value;
}

void Joypad::_handle_hat_changed_event(Hat hat, HatPosition position) {
    if(container::contains(hat_changed_signals_, hat)) {
        for(HatSignalEntry entry: hat_changed_signals_[hat]) {
            entry.second(position, hat);
        }
    }
}

void Joypad::_update() {
    for(std::pair<Axis, int32_t> p: axis_state_) {
        Axis axis = p.first;
        int32_t axis_state = p.second;

        if(axis_state > jitter_value_ || axis_state < -jitter_value_) {
            for(AxisSignalEntry entry: axis_while_nonzero_signals_[axis]) {
                entry.second(float(axis_state) / float(32768), axis);
            }
        }
        if(axis_state > jitter_value_) {
            for(AxisSignalEntry entry: axis_while_above_zero_signals_[axis]) {
                entry.second(float(axis_state) / float(32768), axis);
            }
        }

        if(axis_state < -jitter_value_) {
            for(AxisSignalEntry entry: axis_while_below_zero_signals_[axis]) {
                entry.second(float(axis_state) / float(32768), axis);
            }
        }
    }
}

void Joypad::_disconnect(const InputConnection &connection) {
    for(std::pair<Axis, std::map<InputConnection, JoypadCallback> > p: axis_changed_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }
}

InputController::InputController():
    keyboard_(new Keyboard()) {

    SDL_JoystickEventState(SDL_ENABLE);
    for(uint8_t i = 0; i < SDL_NumJoysticks(); ++i) {
        joypads_.push_back(Joypad::create());
        sdl_joysticks_.push_back(SDL_JoystickOpen(i));
    }
}

InputController::~InputController() {
    //Disable joystick events
    SDL_JoystickEventState(SDL_DISABLE);

    //Make sure we close the joysticks   
    if(SDL_WasInit(SDL_INIT_JOYSTICK)) {
        for(SDL_Joystick* joy: sdl_joysticks_) {
            if(joy) {
                SDL_JoystickClose(joy);
            }
        }
    }
}

void InputController::handle_event(SDL_Event &event) {
    switch(event.type) {
        case SDL_KEYDOWN:
            keyboard()._handle_keydown_event(event.key.keysym);
        break;
        case SDL_KEYUP:
            keyboard()._handle_keyup_event(event.key.keysym);
        break;
        case SDL_JOYAXISMOTION:
            joypad(event.jaxis.which)._handle_axis_changed_event(event.jaxis.axis, event.jaxis.value);
        break;
        case SDL_JOYBUTTONDOWN:
            joypad(event.jbutton.which)._handle_button_down_event(event.jbutton.button);
        break;
        case SDL_JOYBUTTONUP:
            joypad(event.jbutton.which)._handle_button_up_event(event.jbutton.button);
        break;
        case SDL_JOYHATMOTION:
            joypad(event.jhat.which)._handle_hat_changed_event(event.jhat.hat, (HatPosition)event.jhat.value);
        break;
        default:
            break;
    }
}

void InputController::update(double dt) {
    keyboard_->_update(dt);
    for(Joypad::ptr j: joypads_) {
        j->_update();
    }
}

}
