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

#include "../logging.h"
#include "../utils/unicode.h"

#include "input_state.h"

#include "../window.h"
#include "../virtual_gamepad.h"

namespace smlt {

void InputState::_handle_key_down(KeyboardID keyboard_id, KeyboardCode code) {
    if(keyboard_id < keyboard_count_) {
        keyboards_[keyboard_id].keys[code] = true;
    }
}

void InputState::_handle_key_up(KeyboardID keyboard_id, KeyboardCode code) {
    if(keyboard_id < keyboard_count_) {
        keyboards_[keyboard_id].keys[code] = false;
    }
}

void InputState::_handle_mouse_motion(MouseID mouse_id, uint32_t x, uint32_t y, int32_t xrel, int32_t yrel) {
    if(mouse_id < mouse_count_) {
        auto& mouse = mice_[mouse_id];

        mouse.axises[MOUSE_AXIS_X] = xrel;
        mouse.axises[MOUSE_AXIS_Y] = yrel;

        mouse.x = x;
        mouse.y = y;
    }
}

void InputState::_handle_mouse_down(MouseID mouse_id, MouseButtonID button_id) {
    if(mouse_id < mouse_count_) {
        mice_[mouse_id].buttons[button_id] = true;
    }
}

void InputState::_handle_mouse_up(MouseID mouse_id, MouseButtonID button_id) {
    if(mouse_id < mouse_count_) {
        mice_[mouse_id].buttons[button_id] = false;
    }
}

void InputState::_handle_joystick_axis_motion(JoystickID joypad_id, JoystickAxis axis, float value) {
    if(joypad_id < joystick_count_) {
        auto& js = joysticks_[joypad_id];
        js.axises[axis] = value;
    }
}

void InputState::_handle_joystick_button_down(JoystickID joypad_id, JoystickButtonID button_id) {
    if(joypad_id < joystick_count_) {
        auto& js = joysticks_[joypad_id];
        js.buttons[button_id] = true;
    }
}

void InputState::_handle_joystick_button_up(JoystickID joypad_id, JoystickButtonID button_id) {
    if(joypad_id < joystick_count_) {
        auto& js = joysticks_[joypad_id];
        js.buttons[button_id] = false;
    }
}

void InputState::_handle_joystick_hat_motion(JoystickID joypad_id, JoystickHatID hat_id, HatPosition position) {
    if(joypad_id < joystick_count_) {
        auto& js = joysticks_[joypad_id];
        js.hats[hat_id] = position;
    }
}

bool InputState::keyboard_key_state(KeyboardID keyboard_id, KeyboardCode code) const {
    if(keyboard_id < keyboard_count_) {
        return keyboards_[keyboard_id].keys[code];
    }

    return false;
}

bool InputState::mouse_button_state(MouseID mouse_id, JoystickButtonID button) const {
    if(mouse_id < mouse_count_) {
        return mice_[mouse_id].buttons[button];
    }

    return false;
}

bool InputState::joystick_button_state(JoystickID joystick_id, JoystickButtonID button) const {
    if(joystick_id < joystick_count_) {
        return joysticks_[joystick_id].buttons[button];
    }

    return false;
}

float InputState::mouse_axis_state(MouseID mouse_id, MouseAxis axis) const {
    if(mouse_id < mouse_count_) {
        return mice_[mouse_id].axises[axis];
    }

    return 0.0f;
}

float InputState::joystick_axis_state(JoystickID joystick_id, JoystickAxis axis) const {
    if(joystick_id < joystick_count_) {
        return joysticks_[joystick_id].axises[axis];
    }

    return 0.0f;
}

HatPosition InputState::joystick_hat_state(JoystickID joystick_id, JoystickHatID hat) const {
    if(joystick_id < joystick_count_) {
        return joysticks_[joystick_id].hats[hat];
    }

    return HAT_POSITION_CENTERED;
}

void InputState::pre_update(float dt) {
    /* Reset the mouse motion each frame, as motion events only come in
     * when the mouse moves, not every frame */
    for(uint8_t i = 0; i < mouse_count_; ++i) {
        for(uint8_t j = 0; j < mice_[i].axis_count; ++j) {
            mice_[i].axises[j] = 0.0f;
        }
    }
}

void InputState::update(float dt) {

    /*
    if(virtual_joypad_) {
        virtual_joypad_->_update(dt);
    }*/
}

void InputState::init_virtual_joypad() {
    /*
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
    ); */
}


}
