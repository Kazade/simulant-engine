#include <vector>
#include "ui_manager.h"
#include "keyboard.h"
#include "button.h"
#include "frame.h"
#include "../../stage.h"
#include "../../application.h"

namespace smlt {
namespace ui {

const uint16_t SPACE_CHAR = ' ';
const uint16_t BACKSPACE_CHAR = 8;  // ASCII BS
const uint16_t DONE_CHAR = 6;  // ASCII ACK
const uint16_t CASE_TOGGLE_CHAR = 1;  // ASCII Start of Heading

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

static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[16 * 16 * 2 + 1];
} ENTER_ICON = {
  16, 16, 2,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\242\020\323\234\353Z\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\242\020\000\000\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\060\204\333\336(B\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\060\204\276\367\272\326\246\061\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000"
  "\000\000\000\000\000\060\204\276\367\272\326\307\071\000\000\000\000\000\000e)\377\377\363\234\000"
  "\000\000\000\000\000\000\000\000\000\060\204\276\367\377\377\333\336\272\326\272\326\272\326"
  "\272\326\272\326\377\377\363\234\000\000\000\000\000\000\000\000\000\000\060\204\276\367\377\377"
  "\333\336\272\326\272\326\272\326\272\326\272\326\272\326\020\204\000\000\000\000\000"
  "\000\000\000\000\000\000\000\060\204\276\367\272\326\307\071\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\060\204\276\367\272\326\246\061\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\060\204\333\336(B\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\242\020\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
};

static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[16 * 16 * 2 + 1];
} CASE_TOGGLE_ICON = {
  16, 16, 2,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\252R\252R\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\252R\034\347\034\347\252R\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\252"
  "R\373\336\377\377\377\377\373\336\252R\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\252R\034\347}\357}\357}\357}\357\034\347\252R\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\252R\373\336\236\367\256s\373\336\373\336\256s\236\367\373\336\252R\000"
  "\000\000\000\000\000\000\000\000\000\252R\034\347\236\367\317{\000\000\373\336\373\336\000\000\317{"
  "\236\367\034\347\252R\000\000\000\000\000\000\000\000\252R\272\326\256s\000\000\000\000\373\336\373"
  "\336\000\000\000\000\317{\272\326\252R\000\000\000\000\000\000\000\000\000\000E)\000\000\000\000\000\000\373\336"
  "\373\336\000\000\000\000\000\000e)\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\373\336\373"
  "\336\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\373\336\373"
  "\336\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\034\347\034\347"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\317{\317{\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000",
};

static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[16 * 16 * 2 + 1];
} BACKSPACE_ICON = {
  16, 16, 2,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\262"
  "\224\276\367\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\070\306\000\000\000\000,c}\357\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\343"
  "\030y\316\377\377\377\377\377\377\276\367\337\377\377\377\377\377\377\377"
  "}\357\377\377\377\377\377\377\377\377\000\000\064\245\377\377\377\377\377\377"
  "\377\377\262\224\222\224\337\377\377\377<\347,c\070\306\377\377\377\377\377"
  "\377,c\236\367\377\377\377\377\377\377\377\377<\347\014c\222\224\373\336\313"
  "Z\222\224\337\377\377\377\377\377\377\377\232\326\377\377\377\377\377\377"
  "\377\377\377\377\377\377<\347\313Z\246\061\222\224\276\367\377\377\377\377"
  "\377\377\377\377\232\326\377\377\377\377\377\377\377\377\377\377\377\377"
  "<\347\313Z\246\061\222\224\276\367\377\377\377\377\377\377\377\377,c\236\367"
  "\377\377\377\377\377\377\377\377<\347\014c\222\224\373\336\313Z\222\224\337"
  "\377\377\377\377\377\377\377\000\000\064\245\377\377\377\377\377\377\377\377\262"
  "\224\222\224\337\377\377\377<\347,c\070\306\377\377\377\377\377\377\000\000\343"
  "\030y\316\377\377\377\377\377\377\276\367\337\377\377\377\377\377\377\377"
  "}\357\377\377\377\377\377\377\377\377\000\000\000\000,c}\357\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\000\000\000\000\000\000\262\224\276\367\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\070\306\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
};

Keyboard::Keyboard(UIManager *owner, UIConfig *config, KeyboardLayout layout):
    Widget(owner, config),
    layout_(layout) {

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
        [this](const std::pair<char, ButtonInfo>& p) -> bool {
            return p.second.button == focused_;
        }
    );

    if(it != buttons_.end()) {
        auto c = it->first;

        SoftKeyPressedEvent evt;
        evt.character = c;

        signal_key_pressed_(evt);

        if(!evt.cancelled) {
            if(c == CASE_TOGGLE_CHAR) {
                set_mode((mode_ == KEYBOARD_MODE_UPPERCASE) ? KEYBOARD_MODE_LOWERCASE : KEYBOARD_MODE_UPPERCASE);
            } else if(c == DONE_CHAR) {
                signal_done_();
            } else if(target_) {
                auto txt = target_->text();
                if(c == BACKSPACE_CHAR && !txt.empty()) { // Backspace
                    txt = txt.slice(nullptr, -1);
                } else if(c != BACKSPACE_CHAR) { // Any other character
                    txt.push_back(wchar_t(c));
                }
                target_->set_text(txt);
            }
        } else {
            /* Indicate failure by flashing red. A bit of explanation:
             *
             * - state keeps track of the lerp-ing between colours. It's not using a
             * coroutine because we want to keep this lightweight. Either way though handling
             * the case that the keyboard was destroyed while updating is tricksy
             * - So we also hold a weak pointer to alive_marker_. If the keyboard is destroyed
             * we can detect that and bail out immediately
             * - We create a temporary style for this key, so that changing its colour doesn't
             * affect other keys, we then lerp the colour to whatever background it should be
             * (depending on whether the button is focussed or not)
             */
            struct State {
                float t = 0.0f;
                sig::connection conn;
            };

            auto state = std::make_shared<State>();
            auto marker = std::weak_ptr<bool>(alive_marker_);

            auto temp_style = std::make_shared<WidgetStyle>(*buttons_[c].button->style_);
            buttons_[c].button->set_style(temp_style);

            state->conn = get_app()->signal_update().connect([=](float dt) {
                if(!marker.lock()) {
                    /* Keyboard was destroyed, just bail out */
                    state->conn.disconnect();
                    return;
                }

                state->t += (dt * 2.0f);
                if(state->t > 1.0f) {
                    state->t = 1.0f;
                }

                auto current_style = (buttons_[c].button->is_focused()) ? highlighted_style_ : default_style_;

                temp_style->background_colour_ = smlt::Colour::RED.lerp(current_style->background_colour_, state->t);
                buttons_[c].button->rebuild();

                if(state->t == 1.0f) {
                    /* We're done, reset to the correct style - this will destroy our temporary style when we
                     * leave the function */
                    buttons_[c].button->set_style(current_style);

                    /* Disconnect from update() */
                    state->conn.disconnect();
                }
            });
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

void Keyboard::set_mode(KeyboardMode mode) {
    if(mode_ == mode) {
        return;
    }

    mode_ = mode;

    if(layout_ == KEYBOARD_LAYOUT_ALPHABETICAL) {
        generate_alphabetical_layout(mode_ == KEYBOARD_MODE_UPPERCASE);
        buttons_[CASE_TOGGLE_CHAR].button->focus();
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
        if(to_focus) {
            to_focus->focus();
        }
    }
}

void Keyboard::move_left() {
    if(focused_) {
        auto to_focus = focused_->previous_in_focus_chain();
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
    btn->data->stash(value, "enabled");
}

void Keyboard::clear() {
    for(auto& info: buttons_) {
        info.second.button->destroy();
    }
    buttons_.clear();

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
                buttons_[ch].button = new_button(std::string(1, ch));

                if(prev) {
                    prev->set_focus_next(buttons_[ch].button);
                }
                prev = buttons_[ch].button;
                row->pack_child(buttons_[ch].button);
            }

            main_frame_->pack_child(row);
        }
    }

    populate_action_row(rows_[4], ACTION_FLAGS_DEFAULT | ACTION_FLAGS_SPACEBAR);

    main_frame_->pack_child(rows_[4]);
}

smlt::ui::Button* Keyboard::new_button(const unicode& label) {
    Button* btn = nullptr;
    if(default_style_ && highlighted_style_) {
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
    }

    assert(default_style_ != highlighted_style_);

    btn->resize(32, 32);
    btn->signal_focused().connect(std::bind(&Keyboard::focus, this, btn));
    btn->signal_blurred().connect(std::bind(&Keyboard::unfocus, this, btn));
    return btn;
};

static bool mask_set(uint32_t mask, uint32_t c) {
    return (mask & c) == c;
}

void Keyboard::populate_action_row(Frame* target, uint32_t action_flags) {
    /* Now, we add Backspace, space and return */

    if(mask_set(action_flags, ACTION_FLAGS_CASE_TOGGLE)) {
        buttons_[CASE_TOGGLE_CHAR].button = new_button("^");
        buttons_[CASE_TOGGLE_CHAR].button->resize(32, 32);

        auto tex = buttons_[CASE_TOGGLE_CHAR].button->stage->assets->new_texture(CASE_TOGGLE_ICON.width, CASE_TOGGLE_ICON.height, TEXTURE_FORMAT_RGB_1US_565);
        tex->set_data(CASE_TOGGLE_ICON.pixel_data, CASE_TOGGLE_ICON.width * CASE_TOGGLE_ICON.height * CASE_TOGGLE_ICON.bytes_per_pixel);
        tex->convert(
            TEXTURE_FORMAT_RGBA_4UB_8888,
            {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED}
        );
        tex->flush();

        auto widget = stage->ui->new_widget_as_image(tex);
        widget->set_anchor_point(0.5f, 0.5f);
        widget->set_parent(buttons_[CASE_TOGGLE_CHAR].button);
        widget->move_to(16, 0);
        widget->scale_to(1.0f, -1.0f, 1.0f);

        target->pack_child(buttons_[CASE_TOGGLE_CHAR].button);
    }

    if(mask_set(action_flags, ACTION_FLAGS_SPACEBAR)) {
        Px width = rows_[0]->outer_width() - padding().left;
        if(mask_set(action_flags, ACTION_FLAGS_CASE_TOGGLE)) {
            width -= 32;
            width -= target->space_between();
        }

        if(mask_set(action_flags, ACTION_FLAGS_BACKSPACE)) {
            width -= 32;
            width -= target->space_between();
        }

        if(mask_set(action_flags, ACTION_FLAGS_ENTER)) {
            width -= 32;
            width -= target->space_between();
        }

        /* Now, we add space and return */
        buttons_[SPACE_CHAR].button = new_button("");

        auto stage = buttons_[SPACE_CHAR].button->stage.get();
        auto space_tex = stage->assets->new_texture(16, 8, TEXTURE_FORMAT_R_1UB_8);
        space_tex->set_data(SPACE_ICON, 16 * 8);
        space_tex->flip_vertically();
        space_tex->convert(
            TEXTURE_FORMAT_RGBA_4UB_8888,
            {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED}
        );
        space_tex->flush();

        auto space_widget = stage->ui->new_widget_as_image(space_tex);
        space_widget->set_anchor_point(0.5f, 0.5f);
        space_widget->set_parent(buttons_[SPACE_CHAR].button);
        space_widget->move_to(width.value / 2, 0);
        space_widget->scale_to(8.0f, 2.0f, 1.0f);

        if(action_flags & ACTION_FLAGS_CASE_TOGGLE) {
            buttons_[SPACE_CHAR].button->set_focus_previous(buttons_[CASE_TOGGLE_CHAR].button);
        }

        buttons_[SPACE_CHAR].button->resize(width, 32);
        target->pack_child(buttons_[SPACE_CHAR].button);
    }

    if(mask_set(action_flags, ACTION_FLAGS_BACKSPACE)) {
        buttons_[BACKSPACE_CHAR].button = new_button("");
        buttons_[BACKSPACE_CHAR].button->resize(32, 32);

        auto tex = buttons_[BACKSPACE_CHAR].button->stage->assets->new_texture(BACKSPACE_ICON.width, BACKSPACE_ICON.height, TEXTURE_FORMAT_RGB_1US_565);
        tex->set_data(BACKSPACE_ICON.pixel_data, 16 * 16 * 2);
        tex->convert(
            TEXTURE_FORMAT_RGBA_4UB_8888,
            {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED}
        );
        tex->flush();

        auto widget = stage->ui->new_widget_as_image(tex);
        widget->set_anchor_point(0.5f, 0.5f);
        widget->set_parent(buttons_[BACKSPACE_CHAR].button);
        widget->move_to(16, 0);

        if(action_flags & ACTION_FLAGS_SPACEBAR) {
            buttons_[BACKSPACE_CHAR].button->set_focus_previous(buttons_[SPACE_CHAR].button);
        } else if(action_flags & ACTION_FLAGS_CASE_TOGGLE) {
            buttons_[BACKSPACE_CHAR].button->set_focus_previous(buttons_[CASE_TOGGLE_CHAR].button);
        }

        target->pack_child(buttons_[BACKSPACE_CHAR].button);
    }

    if(mask_set(action_flags, ACTION_FLAGS_ENTER)) {
        buttons_[DONE_CHAR].button = new_button(_T(""));
        buttons_[DONE_CHAR].button->resize(32, 32);

        auto tex = buttons_[DONE_CHAR].button->stage->assets->new_texture(ENTER_ICON.width, ENTER_ICON.height, TEXTURE_FORMAT_RGB_1US_565);
        tex->set_data(ENTER_ICON.pixel_data, ENTER_ICON.width * ENTER_ICON.height * ENTER_ICON.bytes_per_pixel);
        tex->convert(
            TEXTURE_FORMAT_RGBA_4UB_8888,
            {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED}
        );
        tex->flush();

        auto widget = stage->ui->new_widget_as_image(tex);
        widget->set_anchor_point(0.5f, 0.5f);
        widget->set_parent(buttons_[DONE_CHAR].button);
        widget->move_to(16, 0);
        widget->scale_to(1.0f, -1.0f, 1.0f);

        if(action_flags & ACTION_FLAGS_BACKSPACE) {
            buttons_[DONE_CHAR].button->set_focus_previous(buttons_[BACKSPACE_CHAR].button);
        } else if(action_flags & ACTION_FLAGS_SPACEBAR) {
            buttons_[DONE_CHAR].button->set_focus_previous(buttons_[SPACE_CHAR].button);
        } else if(action_flags & ACTION_FLAGS_CASE_TOGGLE) {
            buttons_[DONE_CHAR].button->set_focus_previous(buttons_[CASE_TOGGLE_CHAR].button);
        }

        target->pack_child(buttons_[DONE_CHAR].button);
    }
}

void Keyboard::generate_alphabetical_layout(bool uppercase) {
    clear();

    const std::vector<std::string> uppercase_rows = {
        {"1234567890#!"},
        {"ABCDEFGHIJ+_"},
        {"KLMNOPQRST@-"},
        {"UVWXYZ.,<>?*"}
    };

    const std::vector<std::string> lowercase_rows = {
        {"1234567890#!"},
        {"abcdefghij+_"},
        {"klmnopqrst@-"},
        {"uvwxyz.,<>?*"}
    };

    const auto& rows = (uppercase) ? uppercase_rows : lowercase_rows;

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
                buttons_[ch].button = new_button(std::string(1, ch));

                if(prev) {
                    prev->set_focus_next(buttons_[ch].button);
                }
                prev = buttons_[ch].button;
                row->pack_child(buttons_[ch].button);
            }

            main_frame_->pack_child(row);
        }
    }

    populate_action_row(rows_[4], ACTION_FLAGS_BACKSPACE | ACTION_FLAGS_CASE_TOGGLE | ACTION_FLAGS_SPACEBAR | ACTION_FLAGS_ENTER);

    main_frame_->pack_child(rows_[4]);
}

void Keyboard::focus(Widget* widget) {
    widget->set_style(highlighted_style_);
    focused_ = widget;
}

void Keyboard::unfocus(Widget* widget) {
    widget->set_style(default_style_);
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
        button.second.button->set_font(font);
    }
}

}
}
