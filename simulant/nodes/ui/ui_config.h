#pragma once

namespace smlt {
namespace ui {

struct Float4 {
    float left;
    float right;
    float bottom;
    float top;
};

enum OverflowType {
    OVERFLOW_TYPE_HIDDEN,
    OVERFLOW_TYPE_VISIBLE,
    OVERFLOW_TYPE_AUTO
};

enum TextAlignment {
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_CENTER,
    TEXT_ALIGNMENT_RIGHT
};

enum ResizeMode {
    RESIZE_MODE_FIXED, // Clips / scrolls text
    RESIZE_MODE_FIXED_WIDTH, // Will expand vertically with text, text is word-wrapped
    RESIZE_MODE_FIXED_HEIGHT, // Will expand horizontally with text, text is not wrapped
    RESIZE_MODE_FIT_CONTENT // Will fit the text, newlines affect the height
};

enum ChangeFocusBehaviour {
    FOCUS_THIS_IF_NONE_FOCUSED = 0x1,
    FOCUS_NONE_IF_NONE_FOCUSED = 0x2
};

struct UIDim {
    float width = 0.0;
    float height = 0.0;
};

struct UIConfig {
    float font_size_ = 16;
    float line_height_ = 18;

    Colour foreground_colour_ = Colour::BLACK;
    Colour background_colour_ = Colour::WHITE;

    ResizeMode label_resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    ResizeMode button_resize_mode_ = RESIZE_MODE_FIXED_HEIGHT;
    ResizeMode progress_bar_resize_mode_ = RESIZE_MODE_FIXED;

    float scrollbar_width_ = 16;
    Colour scrollbar_background_colour_ = Colour::LIGHT_GREY;
    Colour scrollbar_foreground_colour_ = Colour::ALICE_BLUE;

    float button_height_ = 36;
    float button_width_ = 0; // Fit content

    Float4 label_padding_ = { 5, 5, 5, 5 };
    Colour label_background_colour_ = Colour::NONE;
    Colour label_foreground_colour_ = Colour::NONE;
    Colour label_border_colour_ = Colour::NONE;
    Colour label_text_colour_ = Colour::DODGER_BLUE;

    Float4 button_padding_ = { 30, 30, 20, 20 };
    Colour button_background_colour_ = Colour::DODGER_BLUE;
    Colour button_foreground_colour_ = Colour::NONE;
    Colour button_text_colour_ = Colour::WHITE;
    Colour button_border_colour_ = Colour::NONE;

    float button_border_width_ = 0;
    float button_border_radius_ = 3;

    Colour progress_bar_foreground_colour_ = Colour::DODGER_BLUE;
    Colour progress_bar_background_colour_ = Colour::WHITE;
    Colour progress_bar_border_colour_ = Colour::DODGER_BLUE;
    float progress_bar_border_width_ = 1;
    float progress_bar_height_ = 16.0f;

    OverflowType default_overflow_ = OVERFLOW_TYPE_HIDDEN;
    ResizeMode default_resize_mode_ = RESIZE_MODE_FIXED;
};

}
}
