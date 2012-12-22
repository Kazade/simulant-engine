#ifndef TEXT_CPP
#define TEXT_CPP

#include "scene.h"
#include "text.h"

namespace kglt {

void Text::apply_font(FontID font_id) {
    applied_font_ = font_id;
}

uint32_t Text::length() const {
    return text_.length(); //FIXME: Need to check UTF-8 length
}

double Text::width_in_pixels() {
    return scene().font(applied_font_).string_width_in_pixels(text_);
}

void Text::set_text(const std::string& utf8_text) {
    text_ = utf8_text;
}

Font& Text::font() {
    return scene().font(applied_font_);
}


}

#endif // TEXT_CPP
