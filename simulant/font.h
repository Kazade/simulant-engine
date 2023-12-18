#pragma once

#include <map>

#include "deps/stb_truetype/stb_truetype.h"
#include "types.h"
#include "generic/managed.h"
#include "generic/identifiable.h"
#include "loadable.h"
#include "asset.h"
#include "utils/limited_vector.h"

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
    std::size_t page = 0;
    Vec2 xy0;
    Vec2 xy1;
    Vec2 st0;
    Vec2 st1;
    Vec2 off;
    float xadvance; // Offsets and advance
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


struct FontPage {
    TexturePtr texture;
    MaterialPtr material;
    uint16_t width;
    uint16_t height;
};

class Font:
    public RefCounted<Font>,
    public Asset,
    public Loadable,
    public generic::Identifiable<FontID>,
    public ChainNameable<Font> {

public:
    const static std::size_t max_pages = 4;

    static std::string generate_name(const std::string& family, const uint16_t& size, FontWeight weight, FontStyle style) {
        return family + "-" + font_weight_name(weight) + "-" + font_style_name(style) + "-" + smlt::to_string(size);
    }

    Font(FontID id, AssetManager* asset_manager);

    bool init() override;

    std::size_t page_count() const {
        return pages_.size();
    }

    const FontPage* page(std::size_t i) const;

    std::pair<Vec2, Vec2> char_texcoords(char32_t c) const;
    std::pair<Vec2, Vec2> char_corners(char32_t c) const;

    uint16_t character_width(char32_t ch);
    uint16_t character_height(char32_t ch);
    float character_advance(char32_t ch, char32_t next);
    Vec2 character_offset(char32_t ch);
    uint16_t character_page(char32_t ch);

    uint16_t size() const { return font_size_; }

    int16_t ascent() const;
    int16_t descent() const;
    int16_t line_gap() const;

private:
    /* Given a character, return the width/height of the page it's on */
    uint16_t page_width(char16_t ch) const;
    uint16_t page_height(char16_t ch) const;

    uint16_t font_size_ = 0;
    int16_t ascent_ = 0;
    int16_t descent_ = 0;
    int16_t line_gap_ = 0;
    float scale_ = 0;

    std::unique_ptr<stbtt_fontinfo> info_;
    std::map<char16_t, CharInfo> char_data_;

    LimitedVector<FontPage, max_pages> pages_;

    friend class ui::Widget;
    friend class loaders::TTFLoader;
    friend class loaders::FNTLoader;
};

}
