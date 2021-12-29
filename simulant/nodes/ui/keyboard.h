#pragma once

#include <map>
#include "widget.h"
#include "../../keycodes.h"

namespace smlt {
namespace ui {

/** An onscreen keyboard.
 */

class Frame;

enum KeyboardLayout {
    KEYBOARD_LAYOUT_ALPHABETICAL
};

class Keyboard:
    public Widget {

public:
    using Widget::init; // Pull in init to satisfy Managed<Image>
    using Widget::clean_up;

    Keyboard(UIManager* owner, UIConfig* config);

private:
    KeyboardLayout layout_ = KEYBOARD_LAYOUT_ALPHABETICAL;
    std::map<char, Button*> buttons_;

    Frame* main_frame_ = nullptr;
    Frame* rows_[5] = {0, 0, 0, 0, 0};

    void clear();
    void generate_alphabetical_layout();
};

}
}
