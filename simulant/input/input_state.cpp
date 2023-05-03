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
#include "../macros.h"

#include "input_state.h"

#include "../window.h"

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

void InputState::_handle_joystick_axis_motion(GameControllerID joypad_id, JoystickAxis axis, float value) {
    GameController* controller = game_controller_by_id(joypad_id);
    if(controller) {
        controller->axises[axis] = value;
    } else {
        S_WARN("Recevied input for invalid game controller: {0}", joypad_id.to_int8_t());
    }
}

void InputState::_handle_joystick_button_down(GameControllerID joypad_id, JoystickButton button) {
    GameController* controller = game_controller_by_id(joypad_id);
    if(controller) {
        controller->buttons[button] = true;
    } else {
        S_WARN("Recevied input for invalid game controller: {0}", joypad_id.to_int8_t());
    }
}

void InputState::_handle_joystick_button_up(GameControllerID joypad_id, JoystickButton button) {
    GameController* controller = game_controller_by_id(joypad_id);
    if(controller) {
        controller->buttons[button] = false;
    } else {
        S_WARN("Recevied input for invalid game controller: {0}", joypad_id.to_int8_t());
    }
}

void InputState::_handle_joystick_hat_motion(GameControllerID joypad_id, JoystickHatID hat_id, HatPosition position) {
    GameController* controller = game_controller_by_id(joypad_id);
    if(controller) {
        controller->hats[hat_id] = position;
    } else {
        S_WARN("Recevied input for invalid game controller: {0}", joypad_id.to_int8_t());
    }
}

bool InputState::keyboard_key_state(KeyboardID keyboard_id, KeyboardCode code) const {
    if(keyboard_id < keyboard_count_) {
        return keyboards_[keyboard_id].keys[code];
    }

    return false;
}

bool InputState::mouse_button_state(MouseID mouse_id, MouseButtonID button) const {
    if(mouse_id < mouse_count_) {
        return mice_[mouse_id].buttons[button];
    }

    return false;
}

bool InputState::joystick_button_state(GameControllerID joystick_id, JoystickButton button) const {
    const GameController* controller = game_controller_by_id(joystick_id);
    if(controller) {
        return controller->buttons[button];
    }
    return false;
}

float InputState::mouse_axis_state(MouseID mouse_id, MouseAxis axis) const {
    if(mouse_id < mouse_count_) {
        return mice_[mouse_id].axises[axis];
    }

    return 0.0f;
}

GameController *InputState::game_controller_by_id(GameControllerID id) {
    for(int i = 0; i < joystick_count_; ++i) {
        if(joysticks_[i].id_ == id) {
            return &joysticks_[i];
        }
    }

    return nullptr;
}

const GameController *InputState::game_controller_by_id(GameControllerID id) const {
    for(int i = 0; i < joystick_count_; ++i) {
        if(joysticks_[i].id_ == id) {
            return &joysticks_[i];
        }
    }

    return nullptr;
}

GameController *InputState::game_controller(GameControllerIndex index) {
    auto idx = index.to_int8_t();
    if(idx >= (int8_t) game_controller_count()) {
        return nullptr;
    }

    if(idx < 0) {
        S_ERROR("Received negative index to game_controller()");
        return nullptr;
    }

    return &joysticks_[idx];
}

GameControllerIndex InputState::game_controller_index_from_id(GameControllerID id) const {
    auto idx = 0;
    for(auto& cont: joysticks_) {
        if(cont.id() == id) {
            return GameControllerIndex(idx);
        }

        ++idx;
    }

    return GameControllerIndex(-1);
}

float InputState::joystick_axis_state(GameControllerID joystick_id, JoystickAxis axis) const {
    const GameController* controller = game_controller_by_id(joystick_id);
    if(controller) {
        return controller->axises[axis];
    }
    return 0.0f;
}

HatPosition InputState::joystick_hat_state(GameControllerID joystick_id, JoystickHatID hat) const {
    const GameController* controller = game_controller_by_id(joystick_id);
    if(controller) {
        return controller->hats[hat];
    }

    return HAT_POSITION_CENTERED;
}

void InputState::pre_update(float dt) {
    _S_UNUSED(dt);

    /* Reset the mouse motion each frame, as motion events only come in
     * when the mouse moves, not every frame */
    for(uint8_t i = 0; i < mouse_count_; ++i) {
        for(uint8_t j = 0; j < mice_[i].axis_count; ++j) {
            mice_[i].axises[j] = 0.0f;
        }
    }
}

void InputState::update(float dt) {
    _S_UNUSED(dt);
}

void InputState::_update_game_controllers(const std::vector<GameControllerInfo>& device_info) {
    S_DEBUG("Updating controllers with new list of size: {0}", device_info.size());

    if(device_info.size() > joystick_count_) {
        S_INFO("{0} controllers connected", device_info.size() - joystick_count_);
    } else if(device_info.size() < joystick_count_) {
        S_INFO("{0} controllers removed", joystick_count_ - device_info.size());
    }

    joystick_count_ = std::min(device_info.size(), MAX_DEVICE_TYPE_COUNT);
    for(decltype(joystick_count_) i = 0; i < joystick_count_; ++i) {
        joysticks_[i] = GameController(this, GameControllerID(device_info[i].id));
        joysticks_[i].button_count = device_info[i].button_count;
        joysticks_[i].axis_count = device_info[i].axis_count;
        joysticks_[i].hat_count = device_info[i].hat_count;
        joysticks_[i].has_rumble_ = device_info[i].has_rumble;
        joysticks_[i].platform_data_.i = device_info[i].platform_data.i;
    }
}

JoystickAxis InputState::linked_axis(GameControllerID id, JoystickAxis axis) {
    _S_UNUSED(id);
    if(axis == JOYSTICK_AXIS_X) return JOYSTICK_AXIS_Y;
    if(axis == JOYSTICK_AXIS_Y) return JOYSTICK_AXIS_X;
    return JOYSTICK_AXIS_INVALID;
}

GameControllerIndex GameController::index() const {
    return parent_->game_controller_index_from_id(id_);
}

bool GameController::has_rumble_effect() const {
    return has_rumble_;
}

bool GameController::start_rumble(float low_rumble, float high_rumble, const smlt::Seconds& duration) {
    if(!has_rumble_) {
        return false;
    }

    parent_->window_->game_controller_start_rumble(this, low_rumble, high_rumble, duration);
    return true;
}

void GameController::stop_rumble() {
    parent_->window_->game_controller_stop_rumble(this);
}

bool GameController::button_state(JoystickButton button) const {
    return parent_->joystick_button_state(id_, button);
}

float GameController::axis_state(JoystickAxis axis) const {
    return parent_->joystick_axis_state(id_, axis);
}

HatPosition GameController::hat_state(JoystickHatID hat) const {
    return parent_->joystick_hat_state(id_, hat);
}


}
