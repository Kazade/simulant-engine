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
#include "window_base.h"
#include "virtual_gamepad.h"

namespace smlt {

InputConnection::InputConnection(InputConnectionID id, std::weak_ptr<Device> device):
    generic::Identifiable<InputConnectionID>(id),
    device_(device) {

}

InputConnection Device::new_input_connection() {
    static uint32_t idx = 0;
    return InputConnection(InputConnectionID(++idx), shared_from_this());
}

void InputConnection::disconnect() {
    auto device = device_.lock();
    if(device) {
        device->disconnect(*this);
    }
}

InputConnection Keyboard::key_pressed_connect(GlobalKeyCallback callback) {
    InputConnection c = new_input_connection();

    if(global_key_press_signals_.find(c) != global_key_press_signals_.end()) {
        throw std::logic_error("Something went wrong when generating the connection ID");
    }

    global_key_press_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_pressed_connect(KeyboardCode code, KeyCallback callback) {
    InputConnection c = new_input_connection();    
    key_press_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_while_pressed_connect(GlobalKeyDownCallback callback) {
    InputConnection c = new_input_connection();

    if(global_while_key_pressed_signals_.find(c) != global_while_key_pressed_signals_.end()) {
        throw std::logic_error("Something went wrong when generating the connection ID");
    }

    global_while_key_pressed_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_while_pressed_connect(KeyboardCode code, KeyDownCallback callback) {
    InputConnection c = new_input_connection();
    key_while_down_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_released_connect(GlobalKeyCallback callback) {
    InputConnection c = new_input_connection();
    global_key_release_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_released_connect(KeyboardCode code, KeyCallback callback) {
    InputConnection c = new_input_connection();
    key_release_signals_[code][c] = callback;
    return c;
}

Keyboard::Keyboard() {

}

void Keyboard::_handle_keydown_event(KeyboardCode key) {
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
        if(key_press_signals_.count(key)) {
            for(KeySignalEntry entry: key_press_signals_[key]) {
                entry.second(key);
            }
        }

        //This is inside the IF because, otherwise while_key_pressed events will
        //trigger and we don't want that if the key down was handled... feels dirty
        state_[key] = true;
    }
}

void Keyboard::_handle_keyup_event(KeyboardCode key) {
    bool propagation_stopped = false;

    //First trigger all global handlers
    for(GlobalKeySignalEntry entry: global_key_release_signals_) {
        if(entry.second(key)) {
            propagation_stopped = true;
        }
    }

    if(!propagation_stopped) {
        if(key_release_signals_.count(key)) {
            for(KeySignalEntry entry: key_release_signals_[key]) {
                entry.second(key);
            }
        }
        state_[key] = false;
    }

}

void Keyboard::_update(float dt) {
    for(auto keysym: state_) {
        for(auto conn: global_while_key_pressed_signals_) {
            conn.second(keysym.first, dt);
        }
    }

    for(auto p: key_while_down_signals_) {
        if(key_state(p.first)) {
            for(std::pair<InputConnection, KeyDownCallback> p2: p.second) {
                p2.second(p.first, dt);
            }
        }
    }
}

void Keyboard::_disconnect(const InputConnection &connection) {
    for(auto p: key_press_signals_) {
        if(p.second.count(connection)) {
            p.second.erase(connection);
        }
    }

    for(auto p: key_while_down_signals_) {
        if(p.second.count(connection)) {
            p.second.erase(connection);
        }
    }

    for(auto p: key_release_signals_) {
        if(p.second.count(connection)) {
            p.second.erase(connection);
        }
    }

    global_key_press_signals_.erase(connection);
}

Joypad::Joypad():
    jitter_value_(3) {

}

InputConnection Joypad::axis_changed_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_changed_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_nonzero_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_nonzero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_below_zero_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_below_zero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_above_zero_connect(JoypadAxis axis, JoypadCallback callback) {
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

InputConnection Joypad::button_while_down_connect(Button button, JoypadButtonDownCallback callback) {
    InputConnection c = new_input_connection();
    button_down_signals_[button][c] = callback;
    return c;
}

InputConnection Joypad::hat_changed_connect(Hat hat, JoypadHatCallback callback) {
    InputConnection c = new_input_connection();
    hat_changed_signals_[hat][c] = callback;
    return c;
}

InputConnection Joypad::hat_while_not_centered_connect(Hat hat, JoypadHatDownCallback callback) {
    InputConnection c = new_input_connection();
    hat_while_not_centered_signals_[hat][c] = callback;
    return c;
}

void Joypad::_handle_button_down_event(Button button) {
    if(button_pressed_signals_.count(button)) {
        for(ButtonSignalEntry entry: button_pressed_signals_[button]) {
            entry.second(button);
        }
    }

    button_state_[button] = true;
}

void Joypad::_handle_button_up_event(Button button) {
    if(button_released_signals_.count(button)) {
        for(ButtonSignalEntry entry: button_released_signals_[button]) {
            entry.second(button);
        }
    }

    button_state_[button] = false;
}

void Joypad::_handle_axis_changed_event(JoypadAxis axis, int32_t value) {
    if(axis_changed_signals_.count(axis)) {
        for(AxisSignalEntry entry: axis_changed_signals_[axis]) {
            entry.second(float(value) / float(32768), axis);
        }
    }

    axis_state_[axis] = value;
}

void Joypad::_handle_hat_changed_event(Hat hat, HatPosition position) {
    if(hat_changed_signals_.count(hat)) {
        for(HatSignalEntry entry: hat_changed_signals_[hat]) {
            entry.second(position, hat);
        }
    }

    hat_state_[hat] = position;
}

void Joypad::_update(float dt) {
    for(std::pair<JoypadAxis, int32_t> p: axis_state_) {
        JoypadAxis axis = p.first;
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

    for(std::pair<Hat, HatPosition> p: hat_state_) {
        HatPosition pos = p.second;
        if(p.second != HAT_POSITION_CENTERED) {
            for(auto entry: hat_while_not_centered_signals_[p.first]) {
                entry.second(pos, p.first, dt);
            }
        }
    }

    for(auto state: button_state_) {
        if(state.second) {
            for(auto signal: button_down_signals_[state.first]) {
                signal.second(state.first, dt);
            }
        }
    }
}

void Joypad::_disconnect(const InputConnection &connection) {
    for(std::pair<JoypadAxis, std::map<InputConnection, JoypadCallback> > p: axis_changed_signals_) {
        if(p.second.count(connection)) {
            p.second.erase(connection);
        }
    }
}

InputController::InputController(WindowBase& window):
    window_(window),
    keyboard_(new Keyboard()),
    mouse_(new Mouse()) {

}

InputController::~InputController() {

}

void InputController::_handle_key_down(uint32_t keyboard_id, KeyboardCode code) {
    if(keyboard_id > 0) {
        L_DEBUG("Multiple keyboards not yet supported");
    }

    keyboard()._handle_keydown_event(code);
}

void InputController::_handle_key_up(uint32_t keyboard_id, KeyboardCode code) {
    if(keyboard_id > 0) {
        L_DEBUG("Multiple keyboards not yet supported");
    }

    keyboard()._handle_keyup_event(code);
}

void InputController::_handle_mouse_motion(uint32_t mouse_id, uint32_t x, uint32_t y, uint32_t xrel, uint32_t yrel) {
    if(mouse_id > 0) {
        L_DEBUG("Multiple mice not yet supported");
    }

    mouse()._handle_motion_event(x, y, xrel, yrel);
}

void InputController::_handle_mouse_down(uint32_t mouse_id, uint32_t button_id) {
    if(mouse_id > 0) {
        L_DEBUG("Multiple mice not yet supported");
    }

    assert(0 && "Not implemented");
}

void InputController::_handle_mouse_up(uint32_t mouse_id, uint32_t button_id) {
    if(mouse_id > 0) {
        L_DEBUG("Multiple mice not yet supported");
    }

    assert(0 && "Not implemented");
}

void InputController::_handle_joypad_axis_motion(uint32_t joypad_id, JoypadAxis axis, int32_t value) {
    joypad(joypad_id)._handle_axis_changed_event(axis, value);
}

void InputController::_handle_joypad_button_down(uint32_t joypad_id, uint32_t button_id) {
    joypad(joypad_id)._handle_button_down_event(button_id);
}

void InputController::_handle_joypad_button_up(uint32_t joypad_id, uint32_t button_id) {
    joypad(joypad_id)._handle_button_up_event(button_id);
}

void InputController::_handle_joypad_hat_motion(uint32_t joypad_id, uint32_t hat_id, HatPosition position) {
    joypad(joypad_id)._handle_hat_changed_event(hat_id, position);
}

void InputController::update(float dt) {
    keyboard_->_update(dt);
    mouse_->_update(dt);

    for(Joypad::ptr j: joypads_) {
        j->_update(dt);
    }

    if(virtual_joypad_) {
        virtual_joypad_->_update(dt);
    }
}

void InputController::_update_joypad_devices(const std::vector<GameControllerInfo> &device_info) {
    joypads_.clear();

    while(device_info.size() > joypads_.size()) {
        joypads_.push_back(Joypad::create());
    }
}

void InputController::init_virtual_joypad() {
    if(!virtual_joypad_) {
        virtual_joypad_ = std::make_shared<Joypad>();
    }

    auto& vpad = window_.virtual_joypad;

    for(auto conn: virtual_joypad_connections_) {
        conn.disconnect();
    }
    virtual_joypad_connections_.clear();

    virtual_joypad_connections_.push_back(
        vpad->signal_button_down().connect([=](int btn) {
            joypad(joypads_.size())._handle_button_down_event(btn);
        })
    );

    virtual_joypad_connections_.push_back(
        vpad->signal_button_up().connect([=](int btn) {
            joypad(joypads_.size())._handle_button_up_event(btn);
        })
    );

    virtual_joypad_connections_.push_back(
        vpad->signal_hat_changed().connect([=](HatPosition pos) {
            joypad(joypads_.size())._handle_hat_changed_event(0, pos);
        })
    );
}

Joypad& InputController::joypad(uint8_t idx) {
    if(idx == joypads_.size() && window_.has_virtual_joypad()) {
        init_virtual_joypad();
        return *virtual_joypad_;
    }
    return *joypads_.at(idx);
}

uint8_t InputController::joypad_count() const {
    auto ret = joypads_.size();
    if(window_.has_virtual_joypad()) {
        ++ret;
    }

    return ret;
}

InputConnection Mouse::motion_event_connect(MouseMotionCallback callback) {
    InputConnection c = new_input_connection();
    motion_event_signals_[c] = callback;
    return c;
}

void Mouse::_handle_motion_event(int32_t x, int32_t y, int32_t relx, int32_t rely) {
    last_motion_event_ = MotionEvent(x, y, relx, rely);
}

void Mouse::_disconnect(const InputConnection &connection) {
    motion_event_signals_.erase(connection);
}

void Mouse::_update(float dt) {
    for(auto& entry: this->motion_event_signals_) {
        entry.second(
            last_motion_event_.x,
            last_motion_event_.y,
            last_motion_event_.relx,
            last_motion_event_.rely
        );
    }

    last_motion_event_ = MotionEvent(); //Reset event
}


}
