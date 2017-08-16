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
    void set_return_speed(float ret);

    const std::string& name() const { return name_; }

    const AxisType& type() const { return type_; }

    KeyboardCode positive_keyboard_key() const { return positive_key_; }
    KeyboardCode negative_keyboard_key() const { return negative_key_; }

    void set_keyboard_source(KeyboardID keyboard);
    KeyboardID keyboard_source() const { return keyboard_source_; }

    float value(bool respect_dead_zone=true) const {
        if(respect_dead_zone) {
            return abs(value_) > dead_zone_ ? value_ : 0.0f;
        }

        return value_;
    }

    float dead_zone() const { return dead_zone_; }

private:
    std::string name_;

    AxisType type_ = AXIS_TYPE_KEY_OR_MOUSE_BUTTON;

    KeyboardID keyboard_source_ = ALL_KEYBOARDS;
    KeyboardCode positive_key_ = KEYBOARD_CODE_NONE;
    KeyboardCode negative_key_ = KEYBOARD_CODE_NONE;

    float return_speed_ = 3.0f;
    float value_ = 0.0f;
    float dead_zone_ = 0.001f;

    friend class InputManager;
};

}
