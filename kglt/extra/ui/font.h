#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <cstdint>

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {
namespace ui {

class CharacterInfo {
    uint32_t texture_offset;

};

const int FONT_TEXTURE_SIZE = 512;

class Font :
    public Managed<Font> {
public:
    Font(const std::string& path, uint8_t height);

    TextureID texture_for_char(wchar_t c); //Characters are divided into textures depending on the code
    const CharacterInfo& info_for_char(wchar_t c);

private:
    std::string ttf_file_;
    uint8_t font_height_;

    uint16_t horizontal_glyph_count_;
    uint16_t vertical_glyph_count_;

    std::vector<TextureID> textures_;

    FT_Face face_;
};

}
}
}

#endif // FONT_H
