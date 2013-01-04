#include "kazbase/list_utils.h"
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

InputController::InputController():
    keyboard_(new Keyboard()) {

}

void InputController::handle_event(SDL_Event &event) {
    switch(event.type) {
        case SDL_KEYDOWN:
            keyboard_->_handle_keydown_event((KeyCode)event.key.keysym.sym);
        break;
        case SDL_KEYUP:
            keyboard_->_handle_keyup_event((KeyCode)event.key.keysym.sym);
        break;
        default:
            break;
    }
}

void InputController::update() {
    keyboard_->_update();
}

}
