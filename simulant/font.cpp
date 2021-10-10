#include "font.h"
#include "texture.h"
#include "assets/material.h"
#include "macros.h"

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#define STBTT_STATIC
#define STBTT_memcpy memcpy
#define STBTT_memset memset

#include "deps/stb_truetype/stb_truetype.h"

namespace smlt {

Font::Font(FontID id, AssetManager *asset_manager):
    Asset(asset_manager),
    generic::Identifiable<FontID>(id) {

}

TextureID Font::texture_id() const {
    return texture_->id();
}

MaterialID Font::material_id() const {
    return material_->id();
}

bool Font::init() {

    return true;
}

std::pair<Vec2, Vec2> Font::texture_coordinates_for_character(char32_t ch) {
    if(info_) {
        stbtt_aligned_quad q;
        float x = 0, y = 0;

        stbtt_bakedchar* tmp = (stbtt_bakedchar*) &char_data_[0];

        stbtt_GetBakedQuad(tmp, 512, 512, ch - 32, &x, &y, &q, 1);

        return std::make_pair(
            Vec2(q.s0, q.t0),
            Vec2(q.s1, q.t1)
        );
    } else {
        auto data = char_data_[ch - 32];
        auto pw = float(page_width(ch));
        auto ph = float(page_height(ch));

        return std::make_pair(
            Vec2(float(data.x0) / pw, float(ph - data.y0) / ph),
            Vec2(float(data.x1) / pw, float(ph - data.y1) / ph)
        );
    }
}

float Font::character_width(char32_t ch) {
    if(ch < 32) {
        return 0;
    }

    auto *b = &char_data_.at(ch - 32);
    return b->x1 - b->x0;
}

float Font::character_height(char32_t ch) {
    if(ch < 32) {
        return this->size();
    }

    auto *b = &char_data_.at(ch - 32);
    return b->y1 - b->y0;
}

float Font::character_advance(char32_t ch, char32_t next) {
    _S_UNUSED(next); // FIXME: Kerning!

    if(ch < 32) {
        return 0;
    }

    auto *b = &char_data_.at(ch - 32);
    return b->xadvance;
}

std::pair<float, float> Font::character_offset(char32_t ch) {
    if(ch < 32) {
        return std::make_pair(0, 0);
    }

    auto *b = &char_data_.at(ch - 32);

    return std::make_pair(
        b->xoff,
        -b->yoff
    );
}

float Font::ascent() const {
    return ascent_;
}

float Font::descent() const {
    return descent_;
}

uint16_t Font::page_width(char ch) {
    _S_UNUSED(ch);

    // FIXME: This will need to change when we support multiple pages
    return page_width_;
}

uint16_t Font::page_height(char ch) {
    _S_UNUSED(ch);

    return page_height_;
}


}
