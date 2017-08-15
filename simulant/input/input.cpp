#include "input.h"

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

void InputAxis::set_return_speed(float ret) {
    return_speed_ = ret;
}


}
