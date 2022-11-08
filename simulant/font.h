#pragma once

#include "deps/stb_truetype/stb_truetype.h"
#include "types.h"
#include "generic/managed.h"
#include "generic/identifiable.h"
#include "loadable.h"
#include "asset.h"

struct stbtt_fontinfo;

namespace smlt {

namespace loaders {
    class TTFLoader;
    class FNTLoader;
}

enum CharacterSet {
    CHARACTER_SET_LATIN
};

struct CharInfo{
   uint16_t x0, y0, x1, y1; // Coordinates in the bitmap
   float xoff, yoff, xadvance; // Offsets and advance
};

enum FontStyle {
    FONT_STYLE_NORMAL,
    FONT_STYLE_ITALIC
};

enum FontWeight {
    FONT_WEIGHT_LIGHT,
    FONT_WEIGHT_NORMAL,
    FONT_WEIGHT_BOLD,
    FONT_WEIGHT_BLACK
};

constexpr const char* font_weight_name(FontWeight weight) {
    return (weight == FONT_WEIGHT_NORMAL) ? "Regular": (weight == FONT_WEIGHT_BOLD) ? "Bold" : (weight == FONT_WEIGHT_BLACK) ? "Black" : "Light";
}

constexpr const char* font_style_name(FontStyle style) {
    return (style == FONT_STYLE_NORMAL) ? "Normal" : "Italic";
}


class Font:
    public RefCounted<Font>,
    public Asset,
    public Loadable,
    public generic::Identifiable<FontID>,
    public ChainNameable<Font> {

public:
    static std::string generate_name(const std::string& family, const uint16_t& size, FontWeight weight, FontStyle style) {
        return family + "-" + font_weight_name(weight) + "-" + font_style_name(style) + "-" + smlt::to_string(size);
    }

    Font(FontID id, AssetManager* asset_manager);

    bool init() override;

    bool is_valid() const { return bool(info_) && texture_; }
    TexturePtr texture() const;
    MaterialPtr material() const;

    std::pair<Vec2, Vec2> texture_coordinates_for_character(char32_t c);
    uint16_t character_width(char32_t ch);
    uint16_t character_height(char32_t ch);
    float character_advance(char32_t ch, char32_t next);
    std::pair<int16_t, int16_t> character_offset(char32_t ch);

    uint16_t size() const { return font_size_; }

    int16_t ascent() const;
    int16_t descent() const;
    int16_t line_gap() const;

private:
    /* Given a character, return the width/height of the page it's on */
    uint16_t page_width(char ch);
    uint16_t page_height(char ch);

    uint16_t font_size_ = 0;
    int16_t ascent_ = 0;
    int16_t descent_ = 0;
    int16_t line_gap_ = 0;
    float scale_ = 0;

    // FIXME: This should be replaced when multiple page
    // support happens
    float page_width_ = 0;
    float page_height_ = 0;

    std::unique_ptr<stbtt_fontinfo> info_;
    std::vector<CharInfo> char_data_;

    TexturePtr texture_;
    MaterialPtr material_;

    friend class ui::Widget;
    friend class loaders::TTFLoader;
    friend class loaders::FNTLoader;
};

}
