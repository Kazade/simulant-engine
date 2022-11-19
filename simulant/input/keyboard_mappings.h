#pragma once

#include "../keycodes.h"

namespace smlt {

enum KeyboardLayout {
    KEYBOARD_LAYOUT_UK,
    KEYBOARD_LAYOUT_US,
    KEYBOARD_LAYOUT_JP,
    KEYBOARD_LAYOUT_DE,
    KEYBOARD_LAYOUT_FR,
    KEYBOARD_LAYOUT_IT,
    KEYBOARD_LAYOUT_ES,
    KEYBOARD_LAYOUT_MAX
};


char16_t character_for_keyboard_code(
    KeyboardLayout layout,
    KeyboardCode code,
    bool shift
);

}
