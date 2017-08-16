#pragma once

#include <string>
#include "../generic/managed.h"
#include "../types.h"
#include "../keycodes.h"
#include "../input_controller.h"

namespace smlt {

enum AxisType {
    AXIS_TYPE_KEY_OR_MOUSE_BUTTON
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

    float value() const { return value_; }

private:
    std::string name_;

    AxisType type_ = AXIS_TYPE_KEY_OR_MOUSE_BUTTON;

    KeyboardID keyboard_source_ = ALL_KEYBOARDS;
    KeyboardCode positive_key_ = KEYBOARD_CODE_NONE;
    KeyboardCode negative_key_ = KEYBOARD_CODE_NONE;

    float return_speed_;
    float value_ = 0.0f;

    friend class InputManager;
};

}
