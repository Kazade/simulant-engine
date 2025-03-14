#pragma once

#include <cstdint>
#include "../../color.h"
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

    operator Px() const;
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
    static const Color ALICE_BLUE;
    static const Color LIGHT_GREY;
    static const Color DODGER_BLUE;

    std::string font_family_ = "";  /* Use default */
    Px font_size_ = Px(0); /* Use default */

    Rem line_height_ = Rem(1.5f);

    Color foreground_color_ = Color::from_bytes(40, 40, 40, 255);
    Color background_color_ = Color::from_bytes(53, 53, 53, 255);
    Color text_color_ = Color::from_bytes(219, 219, 219, 255);
    Color highlight_color_ = Color::from_bytes(0, 51, 102, 255);

    ResizeMode label_resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    ResizeMode button_resize_mode_ = RESIZE_MODE_FIT_CONTENT;

    uint8_t scrollbar_width_ = 16;
    Color scrollbar_background_color_ = background_color_;
    Color scrollbar_foreground_color_ = foreground_color_;

    UInt4 label_padding_ = { Px(4), Px(4), Px(4), Px(4) };
    PackedColor4444 label_background_color_ = Color::none();
    PackedColor4444 label_foreground_color_ = Color::none();
    PackedColor4444 label_border_color_ = Color::none();
    PackedColor4444 label_text_color_ = text_color_;

    UInt4 button_padding_ = { Px(30), Px(30), Px(20), Px(20) };
    PackedColor4444 button_background_color_ = highlight_color_;
    PackedColor4444 button_foreground_color_ = Color::none();
    PackedColor4444 button_text_color_ = text_color_;
    PackedColor4444 button_border_color_ = Color::none();

    Px button_border_width_ = Px(0);
    Px button_border_radius_ = Px(4);

    UInt4 image_padding_ = {Px(), Px(), Px(), Px()};
    Px image_border_width_ = Px(0);
    PackedColor4444 image_background_color_ = smlt::Color::white();
    PackedColor4444 image_foreground_color_ = smlt::Color::none();
    PackedColor4444 image_text_color_ = smlt::Color::none();

    PackedColor4444 progress_bar_foreground_color_ = highlight_color_;
    PackedColor4444 progress_bar_background_color_ = background_color_;
    PackedColor4444 progress_bar_border_color_ = foreground_color_;
    PackedColor4444 progress_bar_text_color_ = text_color_;
    Px progress_bar_border_width_ = Px(2);

    PackedColor4444 frame_background_color_ = background_color_;
    PackedColor4444 frame_titlebar_color_ = foreground_color_;
    PackedColor4444 frame_text_color_ = text_color_;
    Px frame_border_width_ = Px(2);
    PackedColor4444 frame_border_color_ = foreground_color_;

    OverflowType default_overflow_ = OVERFLOW_TYPE_HIDDEN;
    ResizeMode default_resize_mode_ = RESIZE_MODE_FIXED;
};

}
}
