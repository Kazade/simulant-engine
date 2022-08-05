#include <vector>
#include "ui_manager.h"
#include "keyboard.h"
#include "button.h"
#include "frame.h"
#include "../../stage.h"

namespace smlt {
namespace ui {

const uint16_t SPACE_CHAR = ' ';
const uint16_t BACKSPACE_CHAR = 8;  // ASCII BS
const uint16_t DONE_CHAR = 6;  // ASCII ACK

const uint8_t SPACE_ICON [] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

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

        if(c == DONE_CHAR) {
            signal_done_();
        } else if(target_) {
            auto txt = target_->text();
            if(c == BACKSPACE_CHAR && !txt.empty()) { // Backspace
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

void Keyboard::limit_chars_to(const unicode& char_list) {
    limited_chars_ = char_list;
    if(limited_chars_.empty()) {
        for(auto it: buttons_) {
            set_enabled(it.second, true);
        }
    } else {
        for(auto it: buttons_) {
            if(it.first == DONE_CHAR) {
                continue;
            }

            auto enable = (limited_chars_.find(it.first) != unicode::npos);
            set_enabled(it.second, enable);
        }
    }
}

void Keyboard::move_up() {
    move_row(-1);
}

void Keyboard::move_down() {
    move_row(1);
}

void Keyboard::move_right() {
    if(focused_) {
        auto to_focus = focused_->next_in_focus_chain();
        while(to_focus && !to_focus->data->get<bool>("enabled")) {
            to_focus = to_focus->next_in_focus_chain();
        }

        if(to_focus) {
            to_focus->focus();
        }
    }
}

void Keyboard::move_left() {
    if(focused_) {
        auto to_focus = focused_->previous_in_focus_chain();
        while(to_focus && !to_focus->data->get<bool>("enabled")) {
            to_focus = to_focus->previous_in_focus_chain();
        }

        if(to_focus) {
            to_focus->focus();
        }
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

void Keyboard::set_enabled(Button* btn, bool value) {
    if(value) {
        btn->set_style(default_style_);
    } else {
        btn->set_style(disabled_style_);
    }

    btn->data->stash(value, "enabled");
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
    buttons_[BACKSPACE_CHAR] = new_button("<--");
    buttons_[BACKSPACE_CHAR]->resize(32 * 3 + (2 * 2), 32);
    rows_[4]->pack_child(buttons_[BACKSPACE_CHAR]);

    /* Now, we add space and return */
    buttons_[SPACE_CHAR] = new_button("");

    auto space_tex = buttons_[SPACE_CHAR]->stage->assets->new_texture(16, 8, TEXTURE_FORMAT_R_1UB_8);
    space_tex->set_data(SPACE_ICON, 16 * 8);
    space_tex->convert(TEXTURE_FORMAT_RGB_3UB_888, {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED});
    space_tex->flush();

    buttons_[SPACE_CHAR]->set_background_image(space_tex);
    buttons_[SPACE_CHAR]->set_focus_previous(buttons_[BACKSPACE_CHAR]);
    buttons_[SPACE_CHAR]->resize(32 * 4 + (2 * 3), 32);
    rows_[4]->pack_child(buttons_[SPACE_CHAR]);

    /* \6 == ACK - we don't use return because we may want multiline input */
    buttons_[DONE_CHAR] = new_button("DONE");
    buttons_[DONE_CHAR]->set_focus_previous(buttons_[SPACE_CHAR]);
    buttons_[DONE_CHAR]->resize(32 * 3 + (2 * 2), 32);
    rows_[4]->pack_child(buttons_[DONE_CHAR]);

    main_frame_->pack_child(rows_[4]);
}

smlt::ui::Button* Keyboard::new_button(const unicode& label) {
    Button* btn = nullptr;
    if(default_style_ && highlighted_style_ && disabled_style_) {
        btn = owner_->new_widget_as_button(label, -1, -1, default_style_);
    } else {
        btn = owner_->new_widget_as_button(label, -1, -1);
    }

    /* We style the style of the first button as the "default" and set
     * this on all subsequent buttons. We steal the style of the second button
     * as "highlight" and apply that on focus */
    if(!default_style_) {
        btn->set_background_colour(UIConfig().background_colour_);
        btn->set_border_colour(smlt::Colour::NONE);
        btn->set_border_width(0);
        btn->set_padding(0);

        default_style_ = btn->style_;
    } else if(!highlighted_style_) {
        btn->set_background_colour(UIConfig().highlight_colour_);
        btn->set_border_colour(smlt::Colour::NONE);
        btn->set_border_width(0);
        btn->set_padding(0);

        highlighted_style_ = btn->style_;

        btn->set_style(default_style_);
    } else if(!disabled_style_) {
        btn->set_background_colour(UIConfig().background_colour_);
        btn->set_border_colour(smlt::Colour::NONE);
        btn->set_border_width(0);
        btn->set_padding(0);
        btn->set_text_colour(UIConfig().background_colour_);

        disabled_style_ = btn->style_;

        btn->set_style(default_style_);
    }

    assert(default_style_ != highlighted_style_);

    btn->resize(32, 32);
    btn->signal_focused().connect(std::bind(&Keyboard::focus, this, btn));
    btn->signal_blurred().connect(std::bind(&Keyboard::unfocus, this, btn));
    return btn;
};

void Keyboard::generate_alphabetical_layout() {
    clear();

    std::vector<std::string> rows = {
        {"1234567890#!"},
        {"ABCDEFGHIJ+_"},
        {"KLMNOPQRST@-"},
        {"UVWXYZ.,<>?*"}
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
    buttons_[BACKSPACE_CHAR] = new_button("<--");
    buttons_[BACKSPACE_CHAR]->resize(32 * 2 + (2 * 1), 32);
    rows_[4]->pack_child(buttons_[BACKSPACE_CHAR]);

    /* Now, we add space and return */
    buttons_[SPACE_CHAR] = new_button("");

    auto space_tex = buttons_[SPACE_CHAR]->stage->assets->new_texture(16, 8, TEXTURE_FORMAT_R_1UB_8);
    space_tex->set_data(SPACE_ICON, 16 * 8);
    space_tex->flip_vertically();
    space_tex->convert(
        TEXTURE_FORMAT_RGBA_4UB_8888,
        {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED}
    );
    space_tex->flush();

    buttons_[SPACE_CHAR]->set_foreground_image(space_tex);
    buttons_[SPACE_CHAR]->set_focus_previous(buttons_[BACKSPACE_CHAR]);
    buttons_[SPACE_CHAR]->resize(32 * 6 + (2 * 5), 32);
    rows_[4]->pack_child(buttons_[SPACE_CHAR]);

    buttons_[DONE_CHAR] = new_button("DONE");
    buttons_[DONE_CHAR]->set_focus_previous(buttons_[SPACE_CHAR]);
    buttons_[DONE_CHAR]->resize(32 * 4 + (2 * 3), 32);
    rows_[4]->pack_child(buttons_[DONE_CHAR]);

    main_frame_->pack_child(rows_[4]);
}

void Keyboard::focus(Widget* widget) {
    widget->set_style(highlighted_style_);
    focused_ = widget;
}

void Keyboard::unfocus(Widget* widget) {
    if(!widget->data->get<bool>("enabled")) {
        widget->set_style(disabled_style_);
    } else {
        widget->set_style(default_style_);
    }

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
    bool rebuild = anchor_point_dirty_;

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
