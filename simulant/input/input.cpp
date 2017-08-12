#include "input.h"

namespace smlt {

Input::Input(const std::string& name):
    name_(name) {

}

void Input::set_positive_keyboard_key(const KeyboardCode& key) {
    positive_key_ = key;
}

void Input::set_negative_keyboard_key(const KeyboardCode& key) {
    negative_key_ = key;
}

void Input::set_return_speed(float ret) {
    return_speed_ = ret;
}


}
