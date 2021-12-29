#include <vector>
#include "ui_manager.h"
#include "keyboard.h"
#include "button.h"
#include "frame.h"

namespace smlt {
namespace ui {

Keyboard::Keyboard(UIManager *owner, UIConfig *config):
    Widget(owner, config) {

    main_frame_ = owner->new_widget_as_frame("");
    main_frame_->set_parent(this);
    main_frame_->set_space_between(2);

    generate_alphabetical_layout();
}

void Keyboard::clear() {
    for(auto& button: buttons_) {
        button.second->destroy();
    }

    for(auto& frame: rows_) {
        if(frame) {
            main_frame_->unpack_child(frame);
        }
        frame = nullptr;
    }
}

void Keyboard::generate_alphabetical_layout() {
    clear();

    std::vector<std::string> rows = {
        {"1234567890"},
        {"ABCDEFGHIJ"},
        {"KLMNOPQRST"},
        {"UVWXYZ.-_/"},
        {""},
    };

    int i = 0;
    for(auto& row: rows_) {
        row = owner_->new_widget_as_frame("");
        row->set_anchor_point(0.5f, 0.5f);
        row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
        row->set_border_width(0);
        row->set_space_between(2);

        for(auto ch: rows[i++]) {
            buttons_[ch] = owner_->new_widget_as_button(std::string(1, ch));
            buttons_[ch]->resize(32, 32);
            buttons_[ch]->set_padding(0);
            buttons_[ch]->set_border_width(0);

            row->pack_child(buttons_[ch]);
        }

        main_frame_->pack_child(row);
    }
}

}
}
