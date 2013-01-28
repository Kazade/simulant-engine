#include "kglt/kazbase/list_utils.h"
#include "kglt/kazbase/unicode/unicode.h"
#include "kglt/kazbase/logging/logging.h"
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

InputConnection Keyboard::key_pressed_connect(KeyCode code, KeyboardCallback callback) {
    InputConnection c = new_input_connection();
    key_press_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_while_down_connect(KeyCode code, KeyboardCallback callback) {
    InputConnection c = new_input_connection();
    key_while_down_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_released_connect(KeyCode code, KeyboardCallback callback) {
    InputConnection c = new_input_connection();
    key_release_signals_[code][c] = callback;
    return c;
}

Keyboard::Keyboard() {
    state_.resize(KEY_CODE_LAST, 0);
}

void Keyboard::_handle_keydown_event(KeyCode key) {
    if(container::contains(key_press_signals_, key)) {
        for(SignalEntry entry: key_press_signals_[key]) {
            entry.second(key);
        }
    }

    state_[key] = true;
}

void Keyboard::_handle_keyup_event(KeyCode key) {
    if(container::contains(key_release_signals_, key)) {
        for(SignalEntry entry: key_release_signals_[key]) {
            entry.second(key);
        }
    }
    state_[key] = false;
}

void Keyboard::_update() {
    for(KeyCode key: container::keys(key_while_down_signals_)) {
        if(state_[key]) {
            for(SignalEntry entry: key_while_down_signals_[key]) {
                entry.second(key);
            }
        }
    }
}

void Keyboard::_disconnect(const InputConnection &connection) {
    for(std::pair<KeyCode, std::map<InputConnection, KeyboardCallback> > p: key_press_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    for(std::pair<KeyCode, std::map<InputConnection, KeyboardCallback> > p: key_while_down_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    for(std::pair<KeyCode, std::map<InputConnection, KeyboardCallback> > p: key_release_signals_) {
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

void Joypad::_update() {
    for(Axis axis: container::keys(axis_state_)) {
        if(axis_state_[axis] > jitter_value_ || axis_state_[axis] < -jitter_value_) {
            for(AxisSignalEntry entry: axis_while_nonzero_signals_[axis]) {
                entry.second(float(axis_state_[axis]) / float(32768), axis);
            }
        }
        if(axis_state_[axis] > jitter_value_) {
            for(AxisSignalEntry entry: axis_while_above_zero_signals_[axis]) {
                entry.second(float(axis_state_[axis]) / float(32768), axis);
            }
        }

        if(axis_state_[axis] <- jitter_value_) {
            for(AxisSignalEntry entry: axis_while_below_zero_signals_[axis]) {
                entry.second(float(axis_state_[axis]) / float(32768), axis);
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
            keyboard()._handle_keydown_event((KeyCode)event.key.keysym.sym);
        break;
        case SDL_KEYUP:
            keyboard()._handle_keyup_event((KeyCode)event.key.keysym.sym);
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
        default:
            break;
    }
}

void InputController::update() {
    keyboard_->_update();
    for(Joypad::ptr j: joypads_) {
        j->_update();
    }
}

}
