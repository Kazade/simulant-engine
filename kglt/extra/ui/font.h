#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <cstdint>
#include <map>

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {
namespace ui {

class Interface;

struct CharacterInfo {
    uint8_t offset_x;
    uint8_t offset_y;
    uint8_t width;
    uint8_t height;
    uint8_t advance_x;

    TextureID texture;
    // bottom-left, bottom-right, top-right, top-left
    kmVec2 texture_coordinates[4];
};

const int FONT_TEXTURE_SIZE = 512;

class Font :
    public Managed<Font> {
public:
    Font(Interface& interface, const std::string& path, uint8_t height);

    const CharacterInfo& info_for_char(wchar_t c);

private:
    Interface& interface_;

    std::string ttf_file_;
    uint16_t font_height_;

    uint16_t current_texture_; //The current texture
    uint16_t current_texture_row_; //The current row, this increments when a glyph won't fit in the row
    uint16_t vertical_texture_rows_; //The number of rows in the texture
    uint16_t row_next_left_; //The position on the row that the next character bitmap will be inserted

    std::vector<TextureID> textures_;
    FT_Face face_;

    std::map<wchar_t, CharacterInfo> info_cache_;
};

}
}
}

#endif // FONT_H
