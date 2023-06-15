#pragma once

#include <cstdint>
#include "../../colour.h"
#include "../../font.h"

namespace smlt {
namespace ui {

enum OverflowType : uint8_t {
    OVERFLOW_TYPE_HIDDEN,
    OVERFLOW_TYPE_VISIBLE,
    OVERFLOW_TYPE_AUTO
};

enum TextAlignment : uint8_t {
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_CENTER,
    TEXT_ALIGNMENT_RIGHT
};

enum ResizeMode : uint8_t {
    RESIZE_MODE_FIXED, // Clips / scrolls text
    RESIZE_MODE_FIXED_WIDTH, // Will expand vertically with text, text is word-wrapped
    RESIZE_MODE_FIXED_HEIGHT, // Will expand horizontally with text, text is not wrapped
    RESIZE_MODE_FIT_CONTENT // Will fit the text, newlines affect the height
};

enum WrapMode : uint8_t {
    WRAP_MODE_CHAR,
    WRAP_MODE_WORD
};

enum ChangeFocusBehaviour : uint8_t {
    FOCUS_THIS_IF_NONE_FOCUSED = 0x1,
    FOCUS_NONE_IF_NONE_FOCUSED = 0x2
};

struct Rem;

/* Absolute pixel value */
struct Px {
    int16_t value = 0;

    Px() = default;

    /* Implicitly convert integer types */
    Px(const int& rhs):
        value(rhs) {}

    Px(const unsigned int& rhs):
        value(rhs) {}

    Px(const uint16_t& rhs):
        value(rhs) {}

    Px(const long unsigned int& rhs):
        value(rhs) {}

    /* Don't convert float types implicitly */
    explicit Px(const double& rhs):
        value(rhs) {}

    explicit Px(const float& rhs):
        value(rhs) {}

    explicit operator bool() const {
        return value != 0;
    }

    Px& operator=(const int rhs) {
        value = rhs;
        return *this;
    }

    Px operator-() const {
        return Px(-value);
    }

    bool operator>=(const Px& rhs) const {
        return value >= rhs.value;
    }

    bool operator>(const Px& rhs) const {
        return value > rhs.value;
    }

    bool operator<(const Px& rhs) const {
        return value < rhs.value;
    }

    bool operator>(const int rhs) const {
        return value > rhs;
    }

    bool operator==(const int16_t rhs) const {
        return value == rhs;
    }

    bool operator==(const Px& rhs) const {
        return value == rhs.value;
    }

    bool operator!=(const Px& rhs) const {
        return !((*this) == rhs);
    }

    bool operator<(const int rhs) const {
        return value < rhs;
    }

    Px operator*(const int rhs) const {
        return Px(value * rhs);
    }

    Px operator/(const int rhs) const {
        return Px(value / rhs);
    }

    Px operator+(const int rhs) const {
        return Px(value + rhs);
    }

    Px operator+(const uint16_t rhs) const {
        return Px(value + rhs);
    }

    Px operator-(const uint16_t rhs) const {
        return Px(value - rhs);
    }

    Px& operator+=(const Px& rhs) {
        value += rhs.value;
        return *this;
    }

    Px& operator-=(const Px& rhs) {
        value -= rhs.value;
        return *this;
    }

    Px operator+(const Px& rhs) const {
        return Px(value + rhs.value);
    }

    Px operator-(const Px& rhs) const {
        return Px(value - rhs.value);
    }

    Px& operator=(const int& rhs) {
        value = rhs;
        return *this;
    }

    Px operator*(const Rem& rhs) const;

    Px operator*(const uint32_t x) const {
        return Px(value * x);
    }

    Px operator/(const uint32_t x) const {
        return Px(value / x);
    }
};

inline bool operator<(const int& lhs, const Px& rhs) {
    return lhs < rhs.value;
}

inline Px operator+(const int& lhs, const Px& rhs) {
    return Px(lhs + rhs.value);
}

inline Px operator-(const int& lhs, const Px& rhs) {
    return Px(lhs - rhs.value);
}

struct UICoord {
    UICoord():
        x(0), y(0) {}

    UICoord(Px x, Px y):
        x(x), y(y) {}

    bool operator==(const UICoord& rhs) const {
        return x == rhs.x && y == rhs.y;
    }

    Px x, y;
};

struct UIDim {
    Px width = Px(0);
    Px height = Px(0);

    UIDim() = default;
    UIDim(Px width, Px height):
        width(width), height(height) {}
};


struct UInt4 {
    Px left;
    Px right;
    Px bottom;
    Px top;
};

inline std::ostream& operator<<(std::ostream& stream, const Px& value) {
    return (stream << value.value);
}

inline bool operator==(const int16_t& lhs, const Px& rhs) {
    return lhs == rhs.value;
}

inline bool operator!=(const int16_t& lhs, const Px& rhs) {
    return lhs != rhs.value;
}

/* Relative to the "root" size */
struct Rem {
    float value = 1.0f;

    Rem() = default;
    explicit Rem(float r):
        value(r) {}
};

/* 100th of the viewport width */
struct Vw {
    float value;
};

/* 100th of the viewport height */
struct Vh {
    float value;
};


extern const char* DEFAULT_FONT_FAMILY;
extern const Px DEFAULT_FONT_SIZE;

struct UIConfig {
    static const Colour ALICE_BLUE;
    static const Colour LIGHT_GREY;
    static const Colour DODGER_BLUE;

    std::string font_family_ = "";  /* Use default */
    Px font_size_ = Px(0); /* Use default */

    Rem line_height_ = Rem(1.5f);

    Colour foreground_colour_ = Colour::from_bytes(40, 40, 40, 255);
    Colour background_colour_ = Colour::from_bytes(53, 53, 53, 255);
    Colour text_colour_ = Colour::from_bytes(219, 219, 219, 255);
    Colour highlight_colour_ = Colour::from_bytes(0, 51, 102, 255);

    ResizeMode label_resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    ResizeMode button_resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    ResizeMode progress_bar_resize_mode_ = RESIZE_MODE_FIXED;

    uint8_t scrollbar_width_ = 16;
    Colour scrollbar_background_colour_ = background_colour_;
    Colour scrollbar_foreground_colour_ = foreground_colour_;

    uint16_t button_height_ = 36;
    uint16_t button_width_ = 0; // Fit content

    UInt4 label_padding_ = { Px(4), Px(4), Px(4), Px(4) };
    PackedColour4444 label_background_colour_ = Colour::NONE;
    PackedColour4444 label_foreground_colour_ = Colour::NONE;
    PackedColour4444 label_border_colour_ = Colour::NONE;
    PackedColour4444 label_text_colour_ = text_colour_;

    UInt4 button_padding_ = { Px(30), Px(30), Px(20), Px(20) };
    PackedColour4444 button_background_colour_ = highlight_colour_;
    PackedColour4444 button_foreground_colour_ = Colour::NONE;
    PackedColour4444 button_text_colour_ = text_colour_;
    PackedColour4444 button_border_colour_ = Colour::NONE;

    Px button_border_width_ = Px(0);
    Px button_border_radius_ = Px(4);

    UInt4 image_padding_ = {Px(), Px(), Px(), Px()};
    Px image_border_width_ = Px(0);
    PackedColour4444 image_background_colour_ = smlt::Colour::WHITE;
    PackedColour4444 image_foreground_colour_ = smlt::Colour::NONE;
    PackedColour4444 image_text_colour_ = smlt::Colour::NONE;

    PackedColour4444 progress_bar_foreground_colour_ = highlight_colour_;
    PackedColour4444 progress_bar_background_colour_ = background_colour_;
    PackedColour4444 progress_bar_border_colour_ = foreground_colour_;
    PackedColour4444 progress_bar_text_colour_ = text_colour_;
    Px progress_bar_border_width_ = Px(2);
    Px progress_bar_width_ = Px(100);
    Rem progress_bar_height_ = Rem(1.5f);

    PackedColour4444 frame_background_colour_ = background_colour_;
    PackedColour4444 frame_titlebar_colour_ = foreground_colour_;
    PackedColour4444 frame_text_colour_ = text_colour_;
    Px frame_border_width_ = Px(2);
    PackedColour4444 frame_border_colour_ = foreground_colour_;

    OverflowType default_overflow_ = OVERFLOW_TYPE_HIDDEN;
    ResizeMode default_resize_mode_ = RESIZE_MODE_FIXED;
};

}
}
