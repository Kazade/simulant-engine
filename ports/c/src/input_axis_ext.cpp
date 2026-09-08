#include "simulant/c/input_axis_ext.h"

#include "simulant/input/input_axis.h"

extern "C" {

void smlt_input_axis_set_positive_keyboard_key_value(smlt_input_axis_t* self, smlt_keyboard_code_t key) {
    reinterpret_cast<smlt::InputAxis*>(self)->set_positive_keyboard_key(
        static_cast<smlt::KeyboardCode>(key));
}

void smlt_input_axis_set_negative_keyboard_key_value(smlt_input_axis_t* self, smlt_keyboard_code_t key) {
    reinterpret_cast<smlt::InputAxis*>(self)->set_negative_keyboard_key(
        static_cast<smlt::KeyboardCode>(key));
}

} // extern "C"
