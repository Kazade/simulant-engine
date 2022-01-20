#include <vector>
#include "ui_manager.h"
#include "keyboard.h"
#include "button.h"
#include "frame.h"

namespace smlt {
namespace ui {

Keyboard::Keyboard(UIManager *owner, UIConfig *config, KeyboardLayout layout):
    Widget(owner, config) {

    main_frame_ = owner->new_widget_as_frame("");
    main_frame_->set_parent(this);
    main_frame_->set_space_between(2);
    main_frame_->set_background_colour(smlt::Colour::NONE);
    main_frame_->set_border_colour(smlt::Colour::NONE);
    main_frame_->set_foreground_colour(smlt::Colour::NONE);

    set_foreground_colour(smlt::Colour::NONE);

    if(layout == KEYBOARD_LAYOUT_ALPHABETICAL) {
        generate_alphabetical_layout();
    } else {
        generate_numerical_layout();
    }

    /* Highlight the first character */
    rows_[0]->packed_children()[0]->focus();
}

Keyboard::~Keyboard() {
    if(target_destroyed_) {
        target_destroyed_.disconnect();
    }
}

void Keyboard::activate() {
    if(!focused_) {
        return;
    }

    auto it = std::find_if(
                buttons_.begin(), buttons_.end(),
        [this](const std::pair<char, Button*>& p) -> bool {
            return p.second == focused_;
        }
    );

    if(it != buttons_.end()) {
        auto c = it->first;

        signal_activated_(c);

        if(c == '\r') {
            /* We use \r to signify done.. rightly or wrongly */
            signal_done_();
        } else if(target_) {
            auto txt = target_->text();
            if(c == 8 && !txt.empty()) { // Backspace
                txt = txt.slice(nullptr, -1);
            } else if(c != 8) { // Any other character
                txt.push_back(wchar_t(c));
            }
            target_->set_text(txt);
        }
    }
}

void Keyboard::set_target(Widget* widget) {
    if(target_destroyed_) {
        target_destroyed_.disconnect();
    }

    target_ = widget;
    target_destroyed_ = target_->signal_destroyed().connect([&]() {
        target_ = nullptr;
        target_destroyed_.disconnect();
    });
}

void Keyboard::move_up() {
    move_row(-1);
}

void Keyboard::move_down() {
    move_row(1);
}

void Keyboard::move_right() {
    if(focused_) {
        focused_->focus_next_in_chain();
    }
}

void Keyboard::move_left() {
    if(focused_) {
        focused_->focus_previous_in_chain();
    }
}

void Keyboard::move_row(int dir) {
    if(!focused_) {
        return;
    }

    int this_row = std::distance(
                std::begin(rows_),
        std::find(std::begin(rows_), std::end(rows_), focused_->parent())
    );

    int i = this_row;
    i += dir;
    i = std::max(0, i);
    i = std::min(4, i);

    if(i != this_row) {
        /* We can move row */
        Px pixels_from_left = 0;
        smlt::ui::Widget* it = focused_->previous_in_focus_chain();
        while(it) {
            pixels_from_left += it->outer_width();
            it = it->previous_in_focus_chain();
        }

        smlt::ui::Widget* new_focussed = nullptr;
        smlt::ui::Frame* new_row = rows_[i];

        Px new_pixels_from_left = 0;
        for(auto& child: new_row->packed_children()) {
            if(!new_focussed) {
                new_focussed = child;
            }

            new_pixels_from_left += child->outer_width();

            if(pixels_from_left.value < new_pixels_from_left.value) {
                new_focussed = child;
                break;
            }
        }
        focused_->blur();
        focused_ = new_focussed;
        focused_->focus();
    }
}

void Keyboard::clear() {
    for(auto& button: buttons_) {
        button.second->destroy();
    }

    for(auto& frame: rows_) {
        if(frame) {
            main_frame_->unpack_child(frame);
            assert(frame->is_destroyed());
        }
        frame = nullptr;
    }
}

void Keyboard::generate_numerical_layout() {
    clear();

    std::vector<std::string> rows = {
        {"+123%"},
        {"-456."},
        {"*789,"},
        {"/:0[]"}
    };

    auto new_button = [this](std::string label) -> smlt::ui::Button* {
        auto btn = owner_->new_widget_as_button(label);
        btn->resize(64, 32);
        btn->set_padding(0);
        btn->set_border_width(0);
        btn->set_background_colour(smlt::Colour::NONE);
        btn->set_border_colour(smlt::Colour::NONE);
        btn->signal_focused().connect(std::bind(&Keyboard::focus, this, btn));
        btn->signal_blurred().connect(std::bind(&Keyboard::unfocus, this, btn));
        return btn;
    };

    smlt::ui::Widget* prev = nullptr;

    for(std::size_t i = 0; i < 5; ++i) {
        auto& row = rows_[i];
        row = owner_->new_widget_as_frame("");
        row->set_anchor_point(0.5f, 0.5f);
        row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
        row->set_border_width(0);
        row->set_space_between(2);
        row->set_background_colour(smlt::Colour::NONE);
        row->set_border_colour(smlt::Colour::NONE);

        if(i < rows.size()) {
            prev = nullptr;
            for(auto ch: rows[i]) {
                buttons_[ch] = new_button(std::string(1, ch));
                if(prev) {
                    prev->set_focus_next(buttons_[ch]);
                }
                prev = buttons_[ch];
                row->pack_child(buttons_[ch]);
            }

            main_frame_->pack_child(row);
        }
    }

    /* Now, we add Backspace, space and return */
    buttons_[8] = new_button("<--");
    buttons_[8]->resize(32 * 3 + (2 * 2), 32);
    rows_[4]->pack_child(buttons_[8]);

    /* Now, we add space and return */
    buttons_[' '] = new_button("___");
    buttons_[' ']->set_focus_previous(buttons_[8]);
    buttons_[' ']->resize(32 * 4 + (2 * 3), 32);
    rows_[4]->pack_child(buttons_[' ']);

    buttons_['\r'] = new_button("DONE");
    buttons_['\r']->set_focus_previous(buttons_[' ']);
    buttons_['\r']->resize(32 * 3 + (2 * 2), 32);
    rows_[4]->pack_child(buttons_['\r']);

    main_frame_->pack_child(rows_[4]);
}

void Keyboard::generate_alphabetical_layout() {
    clear();

    std::vector<std::string> rows = {
        {"1234567890#!"},
        {"ABCDEFGHIJ+_"},
        {"KLMNOPQRST@-"},
        {"UVWXYZ.,<>?*"}
    };

    auto new_button = [this](std::string label) -> smlt::ui::Button* {
        auto btn = owner_->new_widget_as_button(label);
        btn->resize(32, 32);
        btn->set_padding(0);
        btn->set_border_width(0);
        btn->set_background_colour(smlt::Colour::NONE);
        btn->set_border_colour(smlt::Colour::NONE);
        btn->signal_focused().connect(std::bind(&Keyboard::focus, this, btn));
        btn->signal_blurred().connect(std::bind(&Keyboard::unfocus, this, btn));
        return btn;
    };

    smlt::ui::Widget* prev = nullptr;

    for(std::size_t i = 0; i < 5; ++i) {
        auto& row = rows_[i];
        row = owner_->new_widget_as_frame("");
        row->set_anchor_point(0.5f, 0.5f);
        row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
        row->set_border_width(0);
        row->set_space_between(2);
        row->set_background_colour(smlt::Colour::NONE);
        row->set_border_colour(smlt::Colour::NONE);

        if(i < rows.size()) {
            prev = nullptr;
            for(auto ch: rows[i]) {
                buttons_[ch] = new_button(std::string(1, ch));
                if(prev) {
                    prev->set_focus_next(buttons_[ch]);
                }
                prev = buttons_[ch];
                row->pack_child(buttons_[ch]);
            }

            main_frame_->pack_child(row);
        }
    }

    /* Now, we add Backspace, space and return */
    buttons_[8] = new_button("<--");
    buttons_[8]->resize(32 * 2 + (2 * 1), 32);
    rows_[4]->pack_child(buttons_[8]);

    /* Now, we add space and return */
    buttons_[' '] = new_button("___");
    buttons_[' ']->set_focus_previous(buttons_[8]);
    buttons_[' ']->resize(32 * 6 + (2 * 5), 32);
    rows_[4]->pack_child(buttons_[' ']);

    buttons_['\r'] = new_button("DONE");
    buttons_['\r']->set_focus_previous(buttons_[' ']);
    buttons_['\r']->resize(32 * 4 + (2 * 3), 32);
    rows_[4]->pack_child(buttons_['\r']);

    main_frame_->pack_child(rows_[4]);
}

void Keyboard::focus(Widget* widget) {
    widget->set_background_colour(smlt::ui::UIConfig().highlight_colour_);
    focused_ = widget;
}

void Keyboard::unfocus(Widget* widget) {
    widget->set_background_colour(smlt::Colour::NONE);
    focused_ = nullptr;
}

UIDim Keyboard::calculate_content_dimensions(Px text_width, Px text_height) {
    _S_UNUSED(text_width);
    _S_UNUSED(text_height);
    UIDim ret;
    ret.width = main_frame_->outer_width();
    ret.height = main_frame_->outer_height();
    return ret;
}

void Keyboard::on_transformation_change_attempted() {
    bool rebuild = pimpl_->anchor_point_dirty_;

    Widget::on_transformation_change_attempted();

    if(rebuild) {
        /* We need to move all the children */
        main_frame_->set_anchor_point(anchor_point().x, anchor_point().y);
        main_frame_->move_to(0, 0, 0.001f);
    }
}

void Keyboard::set_font(FontPtr font) {
    Widget::set_font(font);
    for(auto& button: buttons_) {
        button.second->set_font(font);
    }
}

}
}
