#pragma once

#include <string>
#include "../generic/managed.h"
#include "../types.h"
#include "../keycodes.h"
#include "../input_controller.h"

namespace smlt {

enum AxisType {
    AXIS_TYPE_KEYBOARD_KEY,
    AXIS_TYPE_MOUSE_BUTTON,
    AXIS_TYPE_MOUSE_AXIS,
    AXIS_TYPE_JOYSTICK_BUTTON,
    AXIS_TYPE_JOYSTICK_AXIS
};

class InputAxis:
    public Managed<InputAxis> {
public:
    InputAxis(const std::string& name);

    void set_positive_keyboard_key(const KeyboardCode& key);
    void set_negative_keyboard_key(const KeyboardCode& key);
    KeyboardCode positive_keyboard_key() const { return positive_key_; }
    KeyboardCode negative_keyboard_key() const { return negative_key_; }

    void set_keyboard_source(KeyboardID keyboard);
    KeyboardID keyboard_source() const { return keyboard_source_; }

    void set_positive_mouse_button(MouseButtonID button);
    void set_negative_mouse_button(MouseButtonID button);
    MouseButtonID positive_mouse_button() const { return positive_mouse_button_; }
    MouseButtonID negative_mouse_button() const { return negative_mouse_button_; }
    void set_mouse_source(MouseID mouse);
    MouseID mouse_source() const { return mouse_source_; }

    void set_positive_joystick_button(JoystickButtonID button);
    void set_negative_joystick_button(JoystickButtonID button);
    JoystickButtonID positive_joystick_button() const { return positive_mouse_button_; }
    JoystickButtonID negative_joystick_button() const { return negative_mouse_button_; }
    void set_joystick_source(JoystickID joystick);
    JoystickID joystick_source() const { return joystick_source_; }

    void set_return_speed(float ret);

    const std::string& name() const { return name_; }

    const AxisType& type() const { return type_; }

    float value(bool respect_dead_zone=true) const;

    float dead_zone() const { return dead_zone_; }

private:
    std::string name_;

    AxisType type_ = AXIS_TYPE_KEYBOARD_KEY;

    KeyboardID keyboard_source_ = ALL_KEYBOARDS;
    KeyboardCode positive_key_ = KEYBOARD_CODE_NONE;
    KeyboardCode negative_key_ = KEYBOARD_CODE_NONE;

    MouseID mouse_source_ = ALL_MICE;
    MouseButtonID positive_mouse_button_ = -1;
    MouseButtonID negative_mouse_button_ = -1;

    JoystickID joystick_source_ = ALL_JOYSTICKS;
    JoystickButtonID positive_joystick_button_ = -1;
    JoystickButtonID negative_joystick_button_ = -1;

    float return_speed_ = 3.0f;
    float value_ = 0.0f;
    float dead_zone_ = 0.001f;

    friend class InputManager;
};

}
