#include "font.h"
#include "texture.h"
#include "material.h"

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#define STBTT_STATIC

#include "deps/stb_truetype/stb_truetype.h"

namespace smlt {

Font::Font(FontID id, ResourceManager *resource_manager):
    Resource(resource_manager),
    generic::Identifiable<FontID>(id) {

}

TextureID Font::texture_id() const { return texture_->id(); }

MaterialID Font::material_id() const { return material_->id(); }

bool Font::init() {

    return true;
}

std::pair<Vec2, Vec2> Font::texture_coordinates_for_character(char32_t ch) {
    stbtt_aligned_quad q;
    float x = 0, y = 0;
    stbtt_GetBakedQuad(&char_data_[0], 512, 512, ch - 32, &x, &y, &q, 1);

    return std::make_pair(
        Vec2(q.s0, q.t0),
        Vec2(q.s1, q.t1)
    );
}

float Font::character_width(char32_t ch) {
    stbtt_aligned_quad q;
    float x = 0, y = 0;
    stbtt_GetBakedQuad(&char_data_[0], 512, 512, ch - 32, &x, &y, &q, 1);

    return q.x1 - q.x0;
}

float Font::character_height(char32_t ch) {
    stbtt_aligned_quad q;
    float x = 0, y = 0;
    stbtt_GetBakedQuad(&char_data_[0], 512, 512, ch - 32, &x, &y, &q, 1);

    return q.y1 - q.y0;
}

float Font::character_advance(char32_t ch, char32_t next) {
    return 0.0f;
}

}
