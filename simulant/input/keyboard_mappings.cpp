#include <cstddef>

#include "keyboard_mappings.h"
#include "../keycodes.h"
#include "../logging.h"

namespace smlt {

struct KeyboardMap {
    KeyboardCode code;
    bool shift;
    char16_t chr;
};

KeyboardMap UK [] = {
    {KEYBOARD_CODE_A, false, 'a'},
    {KEYBOARD_CODE_B, false, 'b'},
    {KEYBOARD_CODE_C, false, 'c'},
    {KEYBOARD_CODE_D, false, 'd'},
    {KEYBOARD_CODE_E, false, 'e'},
    {KEYBOARD_CODE_F, false, 'f'},
    {KEYBOARD_CODE_G, false, 'g'},
    {KEYBOARD_CODE_H, false, 'h'},
    {KEYBOARD_CODE_I, false, 'i'},
    {KEYBOARD_CODE_J, false, 'j'},
    {KEYBOARD_CODE_K, false, 'k'},
    {KEYBOARD_CODE_L, false, 'l'},
    {KEYBOARD_CODE_M, false, 'm'},
    {KEYBOARD_CODE_N, false, 'n'},
    {KEYBOARD_CODE_O, false, 'o'},
    {KEYBOARD_CODE_P, false, 'p'},
    {KEYBOARD_CODE_Q, false, 'q'},
    {KEYBOARD_CODE_R, false, 'r'},
    {KEYBOARD_CODE_S, false, 's'},
    {KEYBOARD_CODE_T, false, 't'},
    {KEYBOARD_CODE_U, false, 'u'},
    {KEYBOARD_CODE_V, false, 'v'},
    {KEYBOARD_CODE_W, false, 'w'},
    {KEYBOARD_CODE_X, false, 'x'},
    {KEYBOARD_CODE_Y, false, 'y'},
    {KEYBOARD_CODE_Z, false, 'z'},
    {KEYBOARD_CODE_A, true, 'A'},
    {KEYBOARD_CODE_B, true, 'B'},
    {KEYBOARD_CODE_C, true, 'C'},
    {KEYBOARD_CODE_D, true, 'D'},
    {KEYBOARD_CODE_E, true, 'E'},
    {KEYBOARD_CODE_F, true, 'F'},
    {KEYBOARD_CODE_G, true, 'G'},
    {KEYBOARD_CODE_H, true, 'H'},
    {KEYBOARD_CODE_I, true, 'I'},
    {KEYBOARD_CODE_J, true, 'J'},
    {KEYBOARD_CODE_K, true, 'K'},
    {KEYBOARD_CODE_L, true, 'L'},
    {KEYBOARD_CODE_M, true, 'M'},
    {KEYBOARD_CODE_N, true, 'N'},
    {KEYBOARD_CODE_O, true, 'O'},
    {KEYBOARD_CODE_P, true, 'P'},
    {KEYBOARD_CODE_Q, true, 'Q'},
    {KEYBOARD_CODE_R, true, 'R'},
    {KEYBOARD_CODE_S, true, 'S'},
    {KEYBOARD_CODE_T, true, 'T'},
    {KEYBOARD_CODE_U, true, 'U'},
    {KEYBOARD_CODE_V, true, 'V'},
    {KEYBOARD_CODE_W, true, 'W'},
    {KEYBOARD_CODE_X, true, 'X'},
    {KEYBOARD_CODE_Y, true, 'Y'},
    {KEYBOARD_CODE_Z, true, 'Z'},
    {KEYBOARD_CODE_0, false, '0'},
    {KEYBOARD_CODE_1, false, '1'},
    {KEYBOARD_CODE_2, false, '2'},
    {KEYBOARD_CODE_3, false, '3'},
    {KEYBOARD_CODE_4, false, '4'},
    {KEYBOARD_CODE_5, false, '5'},
    {KEYBOARD_CODE_6, false, '6'},
    {KEYBOARD_CODE_7, false, '7'},
    {KEYBOARD_CODE_8, false, '8'},
    {KEYBOARD_CODE_9, false, '9'},
    {KEYBOARD_CODE_SEMICOLON, false, ';'},
    {KEYBOARD_CODE_SEMICOLON, true, ':'},
    {KEYBOARD_CODE_APOSTROPHE, false, '\''},
    {KEYBOARD_CODE_APOSTROPHE, true, '@'},
    {KEYBOARD_CODE_NONUSHASH, false, '#'},
    {KEYBOARD_CODE_NONUSHASH, true, '~'},
    {KEYBOARD_CODE_NONUSBACKSLASH, false, '\\'},
    {KEYBOARD_CODE_NONUSBACKSLASH, true, '|'},
    {KEYBOARD_CODE_PERIOD, false, '.'},
    {KEYBOARD_CODE_PERIOD, true, '<'},
    {KEYBOARD_CODE_COMMA, false, ','},
    {KEYBOARD_CODE_COMMA, true, '>'},
    {KEYBOARD_CODE_SLASH, false, '/'},
    {KEYBOARD_CODE_SLASH, true, '?'},
    {KEYBOARD_CODE_LEFTBRACKET, false, '['},
    {KEYBOARD_CODE_LEFTBRACKET, true, '{'},
    {KEYBOARD_CODE_RIGHTBRACKET, false, ']'},
    {KEYBOARD_CODE_RIGHTBRACKET, true, '}'},
};

char16_t find_key(KeyboardMap* map, std::size_t len, KeyboardCode code, bool shift) {
    KeyboardMap* it = map;
    for(std::size_t i = 0; i < len; ++i, ++it) {
        if(it->code == code && it->shift == shift) {
            return it->chr;
        }
    }

    /* Fallback to just code */
    for(std::size_t i = 0; i < len; ++i, ++it) {
        if(it->code == code) {
            return it->chr;
        }
    }

    /* No idea! */
    S_WARN("Couldn't find key for code {0}[{1}]", code, shift);
    return '\0';
}

char16_t character_for_keyboard_code(KeyboardLayout layout,
    KeyboardCode code,
    bool shift
) {

    switch(layout) {
    case KEYBOARD_LAYOUT_UK:
    default:
        return find_key(UK, sizeof(UK), code, shift);
    }
}

}
