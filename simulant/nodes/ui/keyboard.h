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

    void move_up();

    void move_down();

    void move_right() {
        if(focussed_) {
            focussed_->focus_next_in_chain();
        }
    }

    void move_left() {
        if(focussed_) {
            focussed_->focus_previous_in_chain();
        }
    }

private:
    void move_row(int dir);

    KeyboardLayout layout_ = KEYBOARD_LAYOUT_ALPHABETICAL;
    std::map<char, Button*> buttons_;

    Frame* main_frame_ = nullptr;
    Frame* rows_[5] = {0, 0, 0, 0, 0};
    smlt::ui::Widget* focussed_ = nullptr;

    void clear();
    void generate_alphabetical_layout();

    void focus(smlt::ui::Widget* widget);

    void unfocus(smlt::ui::Widget* widget);
};

}
}
