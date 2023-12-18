#include "font.h"
#include "texture.h"
#include "assets/material.h"
#include "macros.h"

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#define STBTT_STATIC
#define STBTT_memcpy memcpy
#define STBTT_memset memset

#include "deps/stb_rect_pack/stb_rect_pack.h"
#include "deps/stb_truetype/stb_truetype.h"

namespace smlt {

const static char FALLBACK_CHAR = '?';

Font::Font(FontID id, AssetManager *asset_manager):
    Asset(asset_manager),
    generic::Identifiable<FontID>(id) {

}

bool Font::init() {

    return true;
}

const FontPage* Font::page(std::size_t i) const {
    assert(i < pages_.size());
    return &pages_[i];
}

std::pair<Vec2, Vec2> Font::char_texcoords(char32_t ch) const {
    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    if(info_) {
        CharInfo q = char_data_.at(ch);
        return std::make_pair(
            q.st0,
            q.st1
        );
    } else {
        auto data = char_data_.at(ch);
        auto pw = float(page_width(ch));
        auto ph = float(page_height(ch));

        return std::make_pair(
            Vec2(float(data.xy0.x) / pw, float(ph - data.xy0.y) / ph),
            Vec2(float(data.xy1.x) / pw, float(ph - data.xy1.y) / ph)
        );
    }
}

std::pair<Vec2, Vec2> Font::char_corners(char32_t ch) const {
    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    CharInfo q = char_data_.at(ch);
    return std::make_pair(q.xy0, q.xy1);
}

uint16_t Font::character_width(char32_t ch) {
    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    auto *b = &char_data_.at(ch);
    return std::abs(b->xy1.x - b->xy0.x);
}

uint16_t Font::character_height(char32_t ch) {
    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    auto *b = &char_data_.at(ch);
    return std::abs(b->xy1.y - b->xy0.y);
}

float Font::character_advance(char32_t ch, char32_t next) {
    _S_UNUSED(next); // FIXME: Kerning!

    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    auto *b = &char_data_.at(ch);
    return b->xadvance;
}

Vec2 Font::character_offset(char32_t ch) {
    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    auto *b = &char_data_.at(ch);    
    return b->off;
}

uint16_t Font::character_page(char32_t ch) {
    if(!char_data_.count(ch)) {
        ch = FALLBACK_CHAR;
    }

    return char_data_[ch].page;
}

int16_t Font::ascent() const {
    return ascent_;
}

int16_t Font::descent() const {
    return descent_;
}

int16_t Font::line_gap() const {
    return line_gap_;
}

uint16_t Font::page_width(char16_t ch) const {
    auto it = char_data_.find(ch);
    if(it == char_data_.end()) {
        it = char_data_.find(FALLBACK_CHAR);
        assert(it != char_data_.end());
    }

    return pages_[it->second.page].width;
}

uint16_t Font::page_height(char16_t ch) const {
    auto it = char_data_.find(ch);
    if(it == char_data_.end()) {
        it = char_data_.find(FALLBACK_CHAR);
        assert(it != char_data_.end());
    }

    return pages_[it->second.page].height;
}


}
