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

class Font:
    public RefCounted<Font>,
    public Asset,
    public Loadable,
    public generic::Identifiable<FontID> {

public:
    Font(FontID id, AssetManager* asset_manager);

    bool init() override;

    bool is_valid() const { return bool(info_) && texture_; }
    TextureID texture_id() const;
    MaterialID material_id() const;

    std::pair<Vec2, Vec2> texture_coordinates_for_character(char32_t c);
    float character_width(char32_t ch);
    float character_height(char32_t ch);
    float character_advance(char32_t ch, char32_t next);
    std::pair<float, float> character_offset(char32_t ch);

    uint32_t size() const { return font_size_; }

    float ascent() const;
    float descent() const;

private:
    /* Given a character, return the width/height of the page it's on */
    uint16_t page_width(char ch);
    uint16_t page_height(char ch);

    uint32_t font_size_ = 0;
    float ascent_ = 0;
    float descent_ = 0;
    float line_gap_ = 0;
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
