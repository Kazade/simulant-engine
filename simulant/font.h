#pragma once

#include "deps/stb_truetype/stb_truetype.h"
#include "types.h"
#include "generic/managed.h"
#include "generic/identifiable.h"
#include "loadable.h"
#include "resource.h"

struct stbtt_fontinfo;

namespace smlt {

namespace loaders {
    class TTFLoader;
}

enum CharacterSet {
    CHARACTER_SET_LATIN
};

class Font:
    public Managed<Font>,
    public Resource,
    public Loadable,
    public generic::Identifiable<FontID> {

public:
    Font(FontID id, ResourceManager* resource_manager);

    bool init() override;

    bool is_valid() const { return bool(info_) && texture_; }
    TextureID texture_id() const;
    MaterialID material_id() const;

    std::pair<Vec2, Vec2> texture_coordinates_for_character(char32_t c);
    float character_width(char32_t ch);
    float character_height(char32_t ch);
    float character_advance(char32_t ch, char32_t next);

private:
    std::unique_ptr<stbtt_fontinfo> info_;
    std::vector<stbtt_bakedchar> char_data_;

    TexturePtr texture_;
    MaterialPtr material_;

    friend class ui::Widget;
    friend class loaders::TTFLoader;
};

}
