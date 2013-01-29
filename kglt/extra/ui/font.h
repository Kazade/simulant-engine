#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <cstdint>
#include <map>
#include "../../kazbase/unicode.h"

#include "../../generic/managed.h"
#include "../../types.h"

namespace kglt {
namespace extra {
namespace ui {

class Interface;

struct CharacterInfo {
    float offset_x;
    float offset_y;
    float width;
    float height;
    float advance_x;

    TextureID texture;
    // bottom-left, bottom-right, top-right, top-left
    kmVec2 texture_coordinates[4];

    //Functions to call to get kerning information
    std::function<int16_t (char32_t)> kern_x;
    std::function<int16_t (char32_t)> kern_y;
};

const int FONT_TEXTURE_SIZE = 512;

class Font :
    public Managed<Font> {
public:
    Font(Interface& interface, const std::string& path, uint8_t height);

    const CharacterInfo& info_for_char(char32_t c);

    unicode family_name() const;
    unicode style_name() const;

    Interface& interface() { return interface_; }
private:
    Interface& interface_;

    std::string ttf_file_;
    uint16_t current_texture_; //The current texture
    uint16_t current_texture_row_; //The current row, this increments when a glyph won't fit in the row
    uint16_t vertical_texture_rows_; //The number of rows in the texture
    uint16_t row_next_left_; //The position on the row that the next character bitmap will be inserted

    std::vector<TextureID> textures_;
    FT_Face face_;

    std::map<char32_t, CharacterInfo> info_cache_;
};

}
}
}

#endif // FONT_H
