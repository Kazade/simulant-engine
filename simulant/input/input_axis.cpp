#include "input_axis.h"

namespace smlt {

InputAxis::InputAxis(const std::string& name):
    name_(name) {

}

void InputAxis::set_positive_keyboard_key(const KeyboardCode& key) {
    positive_key_ = key;
}

void InputAxis::set_negative_keyboard_key(const KeyboardCode& key) {
    negative_key_ = key;
}

void InputAxis::set_positive_mouse_button(MouseButtonID button) {
    positive_mouse_button_ = button;
}

void InputAxis::set_negative_mouse_button(MouseButtonID button) {
    negative_mouse_button_ = button;
}

void InputAxis::set_positive_joystick_button(JoystickButton button) {
    positive_joystick_button_ = button;
}

void InputAxis::set_negative_joystick_button(JoystickButton button) {
    negative_joystick_button_ = button;
}

void InputAxis::set_mouse_source(MouseID mouse) {
    mouse_source_ = mouse;
}

void InputAxis::set_mouse_axis(MouseAxis axis) {
    mouse_axis_ = axis;
}

void InputAxis::set_joystick_axis(JoystickAxis axis) {
    joystick_axis_ = axis;
}

void InputAxis::set_joystick_hat_axis(JoystickHatID hat, JoystickHatAxis axis) {
    joystick_hat_ = hat;
    joystick_hat_axis_ = axis;
}

void InputAxis::set_force(float f) {
    force_ = f;
}

void InputAxis::set_return_speed(float ret) {
    return_speed_ = ret;
}

float InputAxis::value(bool respect_dead_zone) const {
    if(respect_dead_zone) {
        return std::abs(value_) >= dead_zone_ ? value_ : 0.0f;
    }

    return value_;
}


}
