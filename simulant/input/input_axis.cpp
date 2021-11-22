#include "input_axis.h"

namespace smlt {

InputAxis::InputAxis(const std::string& name):
    name_(name) {

}

void InputAxis::set_positive_keyboard_key(const KeyboardCode& key) {
    set_type(AXIS_TYPE_KEYBOARD_KEY);
    positive_key_ = key;
}

void InputAxis::set_negative_keyboard_key(const KeyboardCode& key) {
    set_type(AXIS_TYPE_KEYBOARD_KEY);
    negative_key_ = key;
}

void InputAxis::set_positive_mouse_button(MouseButtonID button) {
    set_type(AXIS_TYPE_MOUSE_BUTTON);
    positive_mouse_button_ = button;
}

void InputAxis::set_negative_mouse_button(MouseButtonID button) {
    set_type(AXIS_TYPE_MOUSE_BUTTON);
    negative_mouse_button_ = button;
}

void InputAxis::set_positive_joystick_button(JoystickButton button) {
    set_type(AXIS_TYPE_JOYSTICK_BUTTON);
    positive_joystick_button_ = button;
}

void InputAxis::set_negative_joystick_button(JoystickButton button) {
    set_type(AXIS_TYPE_JOYSTICK_BUTTON);
    negative_joystick_button_ = button;
}

void InputAxis::set_joystick_source(JoystickID joystick) {
    joystick_source_ = joystick;
}

void InputAxis::set_mouse_source(MouseID mouse) {
    mouse_source_ = mouse;
}

void InputAxis::set_mouse_axis(MouseAxis axis) {
    set_type(AXIS_TYPE_MOUSE_AXIS);
    mouse_axis_ = axis;
}

void InputAxis::set_joystick_axis(JoystickAxis axis) {
    set_type(AXIS_TYPE_JOYSTICK_AXIS);
    joystick_axis_ = axis;
}

void InputAxis::set_joystick_hat_axis(JoystickHatID hat, JoystickHatAxis axis) {
    set_type(AXIS_TYPE_JOYSTICK_HAT);
    joystick_hat_ = hat;
    joystick_hat_axis_ = axis;
}

void InputAxis::set_force(float f) {
    force_ = f;
}

void InputAxis::set_return_speed(float ret) {
    return_speed_ = ret;
}

float InputAxis::value(DeadZoneBehaviour dead_zone_behaviour) const {
    switch(dead_zone_behaviour) {
    case DEAD_ZONE_BEHAVIOUR_NONE:
        return value_;
    default:
    case DEAD_ZONE_BEHAVIOUR_RADIAL: {
        if(type_ == AXIS_TYPE_JOYSTICK_AXIS) {
            Vec2 input(value_, linked_value_);
            float dead_zone_squared = dead_zone_ * dead_zone_;
            if(input.length_squared() < dead_zone_squared) {
                return 0.0f;
            } else {
                /* FIXME: Costly divide */
                input = input.normalized() * ((input.length() - dead_zone_) / (1.0f - dead_zone_));
                return input.x;
            }
        }
    } /* fall through */
    case DEAD_ZONE_BEHAVIOUR_AXIAL:
        return std::abs(value_) >= dead_zone_ ? value_ : 0.0f;
    }
}

void InputAxis::set_type(AxisType type) {
    if(type_ != AXIS_TYPE_UNSET && type != type_) {
        /* We changed from some type to another, this is an unusual
         * behaviour and likely an error, so log a warning */
        S_WARN("Changed axis type for existing axis. This is probably a mistake.");
    }

    type_ = type;
}


}
