#include "input_axis.h"
#include "input_manager.h"
#include "input_state.h"
#include "../macros.h"

namespace smlt {

const static float DEADZONE = 0.2f;

InputManager::InputManager(InputState *controller):
    controller_(controller) {

    auto horizontal = new_axis("Horizontal");
    horizontal->set_positive_keyboard_key(KEYBOARD_CODE_D);
    horizontal->set_negative_keyboard_key(KEYBOARD_CODE_A);

    auto horizontal_alt = new_axis("Horizontal");
    horizontal_alt->set_positive_keyboard_key(KEYBOARD_CODE_RIGHT);
    horizontal_alt->set_negative_keyboard_key(KEYBOARD_CODE_LEFT);

    auto horizontal_js = new_axis("Horizontal");
    horizontal_js->set_type(AXIS_TYPE_JOYSTICK_AXIS);
    horizontal_js->set_joystick_axis(JOYSTICK_AXIS_X);
    horizontal_js->set_dead_zone(DEADZONE);

    auto vertical = new_axis("Vertical");
    vertical->set_positive_keyboard_key(KEYBOARD_CODE_W);
    vertical->set_negative_keyboard_key(KEYBOARD_CODE_S);

    auto vertical_alt = new_axis("Vertical");
    vertical_alt->set_positive_keyboard_key(KEYBOARD_CODE_UP);
    vertical_alt->set_negative_keyboard_key(KEYBOARD_CODE_DOWN);

    auto vertical_js = new_axis("Vertical");
    vertical_js->set_type(AXIS_TYPE_JOYSTICK_AXIS);
    vertical_js->set_joystick_axis(JOYSTICK_AXIS_Y);
    vertical_js->set_dead_zone(DEADZONE);

    auto fire1 = new_axis("Fire1");
    fire1->set_positive_keyboard_key(KEYBOARD_CODE_LCTRL);

    auto fire1_js = new_axis("Fire1");
    fire1_js->set_type(AXIS_TYPE_JOYSTICK_BUTTON);
    fire1_js->set_positive_joystick_button(JOYSTICK_BUTTON_A);

    auto fire1_ms = new_axis("Fire1");
    fire1_ms->set_positive_mouse_button(MouseButtonID(0));

    auto fire2 = new_axis("Fire2");
    fire2->set_positive_keyboard_key(KEYBOARD_CODE_RALT);

    auto fire2_js = new_axis("Fire2");
    fire2_js->set_type(AXIS_TYPE_JOYSTICK_BUTTON);
    fire2_js->set_positive_joystick_button(JOYSTICK_BUTTON_B);

    auto fire2_ms = new_axis("Fire2");
    fire2_ms->set_positive_mouse_button(MouseButtonID(1));

    auto start = new_axis("Start");
    start->set_positive_keyboard_key(KEYBOARD_CODE_RETURN);

    auto start_js = new_axis("Start");
    start_js->set_type(AXIS_TYPE_JOYSTICK_BUTTON);
    start_js->set_positive_joystick_button(JOYSTICK_BUTTON_START);

    auto mouse_x = new_axis("MouseX");
    mouse_x->set_type(AXIS_TYPE_MOUSE_AXIS);
    mouse_x->set_mouse_axis(MOUSE_AXIS_0);

    auto mouse_y = new_axis("MouseY");
    mouse_y->set_type(AXIS_TYPE_MOUSE_AXIS);
    mouse_y->set_mouse_axis(MOUSE_AXIS_1);
}

InputAxis* InputManager::new_axis(const std::string& name) {
    auto axis = InputAxis::create(name);
    axises_.insert(std::make_pair(name, axis));
    return axis.get();
}

AxisList InputManager::axises(const std::string& name) {
    AxisList result;

    auto it1 = axises_.lower_bound(name);
    auto it2 = axises_.upper_bound(name);

    while(it1 != it2) {
        result.push_back(it1->second.get());
        ++it1;
    }

    return result;
}

void InputManager::each_axis(EachAxisCallback callback) {
    for(auto& axis: axises_) {
        callback(axis.second.get());
    }
}

void InputManager::destroy_axises(const std::string& name) {
    axises_.erase(name);
}

void InputManager::destroy_axis(InputAxis* axis) {
    for(auto it = axises_.begin(), last = axises_.end(); it != last; ) {
        if(it->second.get() == axis) {
            it = axises_.erase(it);
        } else {
            ++it;
        }
    }
}

std::size_t InputManager::axis_count(const std::string &name) const {
    return axises_.count(name);
}

inline static float sgn(float v) {
    return (v > 0) ? 1.0f : -1.0f;
}

void InputManager::_process_mouse(int8_t id, int8_t pbtn, int8_t nbtn, bool *positive_pressed, bool *negative_pressed) {
    if(pbtn != -1 && controller_->mouse_button_state(id, pbtn)) {
        *positive_pressed = true;
    }

    if(nbtn != -1 && controller_->mouse_button_state(id, nbtn)) {
        *negative_pressed = true;
    }
}

bool InputManager::_update_mouse_button_axis(InputAxis* axis, float dt) {
    float new_value = axis->value_;

    auto pbtn = axis->positive_mouse_button();
    auto nbtn = axis->negative_mouse_button();

    bool positive_pressed = false;
    bool negative_pressed = false;

    // If the user requested input from all mice, do that
    if(axis->mouse_source() == ALL_MICE) {
        for(std::size_t i = 0; i < controller_->mouse_count(); ++i) {
            MouseID id = (MouseID) i;
            _process_mouse(id, pbtn, nbtn, &positive_pressed, &negative_pressed);
        }
    } else {
        // Otherwise just check the one they asked for
        MouseID id = axis->mouse_source();
        _process_mouse(id, pbtn, nbtn, &positive_pressed, &negative_pressed);
    }

    // If either positive or negative were pressed, adjust the value
    if(positive_pressed) {
        new_value = std::min(1.0f, new_value + (axis->force_ * dt));
    }

    if(negative_pressed) {
        new_value = std::max(-1.0f, new_value - (axis->force_ * dt));
    }

    // If neither were pressed, then apply the return speed (making sure we don't pass zero)
    if(!positive_pressed && !negative_pressed) {
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (std::abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
    }

    axis->value_ = new_value;

    return negative_pressed || positive_pressed;
}

void InputManager::_process_joystick(int8_t id, JoystickButton pbtn, JoystickButton nbtn, bool *positive_pressed, bool *negative_pressed) {
    auto controller = controller_->game_controller(id);
    if(pbtn != -1 && controller->button_state(pbtn)) {
        *positive_pressed = true;
    }

    if(nbtn != -1 && controller->button_state(nbtn)) {
        *negative_pressed = true;
    }
};

bool InputManager::_update_joystick_button_axis(InputAxis* axis, float dt) {
    float new_value = axis->value_;

    auto pbtn = axis->positive_joystick_button();
    auto nbtn = axis->negative_joystick_button();

    bool positive_pressed = false;
    bool negative_pressed = false;

    // If the user requested input from all game controllers, do that
    if(axis->joystick_source() == ALL_JOYSTICKS) {
        for(std::size_t i = 0; i < controller_->game_controller_count(); ++i) {
            _process_joystick(i, pbtn, nbtn, &positive_pressed, &negative_pressed);
        }
    } else {
        // Otherwise just check the one they asked for
        uint8_t id = axis->joystick_source();
        _process_joystick(id, pbtn, nbtn, &positive_pressed, &negative_pressed);
    }

    // If either positive or negative were pressed, adjust the value
    if(positive_pressed) {
        new_value = std::min(1.0f, new_value + (axis->force_ * dt));
    }

    if(negative_pressed) {
        new_value = std::max(-1.0f, new_value - (axis->force_ * dt));
    }

    // If neither were pressed, then apply the return speed (making sure we don't pass zero)
    if(!positive_pressed && !negative_pressed) {
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (std::abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
    }

    axis->value_ = new_value;

    return negative_pressed || positive_pressed;
}

void InputManager::_process_keyboard(int8_t id, KeyboardCode pkey, KeyboardCode nkey, bool *positive_pressed, bool *negative_pressed) {
    if(pkey != KEYBOARD_CODE_NONE && controller_->keyboard_key_state(id, pkey)) {
        *positive_pressed = true;
    }

    if(nkey != KEYBOARD_CODE_NONE && controller_->keyboard_key_state(id, nkey)) {
        *negative_pressed = true;
    }
}

bool InputManager::_update_keyboard_axis(InputAxis* axis, float dt) {
    float new_value = axis->value_;

    auto pkey = axis->positive_keyboard_key();
    auto nkey = axis->negative_keyboard_key();

    bool positive_pressed = false;
    bool negative_pressed = false;

    // If the user requested input from all keyboards, do that
    if(axis->keyboard_source() == ALL_KEYBOARDS) {
        for(std::size_t i = 0; i < controller_->keyboard_count(); ++i) {
            KeyboardID id = (KeyboardID) i;
            _process_keyboard(id, pkey, nkey, &positive_pressed, &negative_pressed);
        }
    } else {
        // Otherwise just check the one they asked for
        KeyboardID id = axis->keyboard_source();
        _process_keyboard(id, pkey, nkey, &positive_pressed, &negative_pressed);
    }

    // If either positive or negative were pressed, adjust the value
    if(positive_pressed) {
        new_value = std::min(1.0f, new_value + (axis->force_ * dt));
    }

    if(negative_pressed) {
        new_value = std::max(-1.0f, new_value - (axis->force_ * dt));
    }

    // If neither were pressed, then apply the return speed (making sure we don't pass zero)
    if(!positive_pressed && !negative_pressed) {
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (std::abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
    }

    axis->value_ = new_value;

    /* Return true if either is pressed */
    return negative_pressed || positive_pressed;
}

void InputManager::update(float dt) {

    /* Reset axis states */
    for(auto& it: axis_states_) {
        prev_axis_states_[it.first] = it.second;
        it.second = false;
    }

    for(auto& axis: axises_) {
        const auto& name = axis.first;

        bool new_state = false;
        auto axis_ptr = axis.second.get();
        auto type = axis_ptr->type();

        if(type == AXIS_TYPE_KEYBOARD_KEY) {
            new_state |= _update_keyboard_axis(axis_ptr, dt);
        } else if(type == AXIS_TYPE_MOUSE_BUTTON) {
            new_state |= _update_mouse_button_axis(axis_ptr, dt);
        } else if(type == AXIS_TYPE_JOYSTICK_BUTTON) {
            new_state |= _update_joystick_button_axis(axis_ptr, dt);
        } else if(type == AXIS_TYPE_MOUSE_AXIS) {
            _update_mouse_axis_axis(axis_ptr, dt);
        } else if(type == AXIS_TYPE_JOYSTICK_AXIS) {
            new_state |= _update_joystick_axis_axis(axis_ptr, dt);
        } else if(type == AXIS_TYPE_JOYSTICK_HAT) {
            new_state |= _update_joystick_hat_axis(axis_ptr, dt);
        }

        /* We may have already set this axis name once this frame, we only
         * want to set to true, never reset back to false here */
        axis_states_[name] = axis_states_[name] || new_state;
    }
}

void InputManager::_update_mouse_axis_axis(InputAxis *axis, float dt) {
    _S_UNUSED(dt);

    float new_value = 0.0f;

    auto process_mouse = [this, axis](MouseID mouse_id) {
        // We just store the mouse axis state directly, this will be a relative delta
        // from the last frame for either mouse movement, or scroll wheel
        return controller_->mouse_axis_state(mouse_id, axis->mouse_axis_);
    };

    // If the source is *all* mice, store the strongest axis (whether positive or negative)
    if(axis->mouse_source() == ALL_MICE) {
        for(std::size_t i = 0; i < controller_->mouse_count(); ++i) {
            auto this_value = process_mouse((MouseID) i);
            if(std::abs(this_value) > std::abs(new_value)) {
                new_value = this_value;
            }
        }
    } else {
        new_value = process_mouse(axis->mouse_source());
    }

    axis->value_ = new_value;
}

bool InputManager::_update_joystick_axis_axis(InputAxis* axis, float dt) {
    _S_UNUSED(dt);

    float new_value = 0.0f;

    auto process_joystick = [this, axis](uint8_t joystick_id) {
        auto controller = controller_->game_controller(joystick_id);
        return controller->axis_state(axis->joystick_axis_);
    };

    uint8_t joystick_used = axis->joystick_source();

    // If the source is *all* joysticks, store the strongest axis (whether positive or negative)
    if(axis->joystick_source() == ALL_JOYSTICKS) {
        for(uint8_t i = 0; i < controller_->game_controller_count(); ++i) {
            auto this_value = process_joystick(i);
            if(std::abs(this_value) >= std::abs(new_value)) {
                new_value = this_value;
                joystick_used = i;
            }
        }
    } else {
        new_value = process_joystick(axis->joystick_source());
    }

    axis->value_ = new_value;

    /* FIXME: Code some API to find the "linked_axis" for an axis and use
     * that if there is one. This currently will only work on the first joystick */
    auto linked_axis = controller_->linked_axis(GameControllerID(), axis->joystick_axis_);
    if(linked_axis != JOYSTICK_AXIS_INVALID) {
        auto controller = controller_->game_controller(joystick_used);
        if(controller) {
            axis->linked_value_ = controller->axis_state(linked_axis);
        } else {
            axis->linked_value_ = 0;
        }
    } else {
        axis->linked_value_ = 0;
    }

    /* Anything in the deadzone returns 0.0f exactly */
    /* FIXME: Radial behaviour here is unchangable, that might not be what
     * the user wants for determining if a joystick was activated */
    return axis->value(DEAD_ZONE_BEHAVIOUR_RADIAL) != 0.0f;
}

bool InputManager::_update_joystick_hat_axis(InputAxis* axis, float dt) {
    float new_value = 0.0f;

    auto process_joystick = [this, axis](uint8_t joystick_id) {
        auto controller = controller_->game_controller(joystick_id);
        auto state = controller->hat_state(axis->joystick_hat_);

        if(axis->joystick_hat_axis_ == JOYSTICK_HAT_AXIS_X) {
            switch(state) {
                case HAT_POSITION_LEFT:
                case HAT_POSITION_LEFT_UP:
                case HAT_POSITION_LEFT_DOWN: return -1.0f;
                case HAT_POSITION_RIGHT:
                case HAT_POSITION_RIGHT_UP:
                case HAT_POSITION_RIGHT_DOWN: return -1.0f;
                default:
                    return 0.0f;
            }
        } else if(axis->joystick_hat_axis_ == JOYSTICK_HAT_AXIS_Y) {
            switch(state) {
                case HAT_POSITION_UP:
                case HAT_POSITION_LEFT_UP:
                case HAT_POSITION_RIGHT_UP: return 1.0f;
                case HAT_POSITION_DOWN:
                case HAT_POSITION_LEFT_DOWN:
                case HAT_POSITION_RIGHT_DOWN: return -1.0f;
                default:
                    return 0.0f;
            }
        }

        return 0.0f;
    };

    // If the source is *all* joysticks, store the strongest axis (whether positive or negative)
    if(axis->joystick_source() == ALL_JOYSTICKS) {
        for(std::size_t i = 0; i < controller_->game_controller_count(); ++i) {
            auto this_value = process_joystick(i);
            if(std::abs(this_value) > std::abs(new_value)) {
                new_value = this_value;
            }
        }
    } else {
        new_value = process_joystick(axis->joystick_source());
    }

    bool ret = true;
    if(new_value == 0.0f) {
        // Neither direction pressed. Use return speed to reset to zero
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (std::abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
        ret = false;
    }

    axis->value_ = new_value;
    return ret;
}

/* Returns the axis value but rounded to -1, 0 or +1. Values in the dead zone will return 0 */
int8_t InputManager::axis_value_hard(const std::string& name) const {
    float v = axis_value(name);
    if(v > 0.0f) {
        return 1;
    } else if(v < 0.0f) {
        return -1;
    }

    return 0;
}

float InputManager::axis_value(const std::string& name) const {
    auto f = 0.0f;

    auto it1 = axises_.lower_bound(name);
    auto it2 = axises_.upper_bound(name);

    while(it1 != it2) {
        auto v = it1->second->value();

        // Return the result with the greatest overall value (positive or negative)
        if(std::abs(v) > std::abs(f)) {
            f = v;
        }

        ++it1;
    }

    return f;
}

bool InputManager::axis_was_pressed(const std::string &name) const {
    auto a = prev_axis_states_.find(name);
    auto b = axis_states_.find(name);

    bool a_active = (a != prev_axis_states_.end()) && a->second;
    bool b_active = (b != axis_states_.end()) && b->second;

    return !a_active && b_active;
}

bool InputManager::axis_was_released(const std::string& name) const {
    auto a = prev_axis_states_.find(name);
    auto b = axis_states_.find(name);

    bool a_active = (a != prev_axis_states_.end()) && a->second;
    bool b_active = (b != axis_states_.end()) && b->second;

    return a_active && !b_active;
}

}
