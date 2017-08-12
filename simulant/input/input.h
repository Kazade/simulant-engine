#pragma once

#include <string>
#include "../types.h"
#include "../keycodes.h"

namespace smlt {

class InputAxis {
public:
    InputAxis(const std::string& name);

    void set_positive_keyboard_key(const KeyboardCode& key);
    void set_negative_keyboard_key(const KeyboardCode& key);
    void set_return_speed(float ret);

private:
    std::string name_;

    KeyboardCode positive_key_ = KEYBOARD_CODE_NONE;
    KeyboardCode negative_key_ = KEYBOARD_CODE_NONE;

    float return_speed_;
};

}
