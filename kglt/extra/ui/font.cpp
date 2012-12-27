#include <freetype/ftglyph.h>
#include <freetype/ftlcdfil.h>

#include <tr1/functional>

#include "kazbase/exceptions.h"
#include "kazbase/list_utils.h"

#include "font.h"
#include "interface.h"

#include "../../scene.h"
#include "../../texture.h"

namespace kglt {
namespace extra {
namespace ui{

struct FreeTypeInitializer {
    /*
     *  Makes sure that FreeType is initialized and destroyed only once
     */
    FreeTypeInitializer() {
        FT_Init_FreeType(&ftlib);
        FT_Library_SetLcdFilter(ftlib, FT_LCD_FILTER_LIGHT);
    }

    ~FreeTypeInitializer() {
        FT_Done_FreeType(ftlib);
    }

    FT_Library ftlib;
};

static FreeTypeInitializer ft;

uint16_t get_kerning_x(FT_Face face, char32_t left, char32_t right) {
    FT_Vector result;
    FT_Get_Kerning(face, left, right, FT_KERNING_DEFAULT, &result);
    return result.x;
}

Font::Font(Interface& interface, const std::string &path, uint8_t height):
    interface_(interface),
    ttf_file_(path),
    font_height_(height),    
    current_texture_(0),
    current_texture_row_(0),
    vertical_texture_rows_(FONT_TEXTURE_SIZE / height),
    row_next_left_(1) {

    if(FT_New_Face(ft.ftlib, ttf_file_.c_str(), 0, &face_) != 0) {
        throw IOError("Unable to load font: " + path);
    }

    FT_Set_Char_Size(face_, height << 6, height << 6, 96, 96);

    Texture& tex = interface.scene().texture(interface.scene().new_texture());
    tex.resize(FONT_TEXTURE_SIZE, FONT_TEXTURE_SIZE);

    textures_.push_back(
        tex.id()
    );
}

const CharacterInfo& Font::info_for_char(char32_t c) {
    /*
     * Loads a glyph and stores it in a texture. Returns all the data
     * needed to render the glyph.
     */

    if(container::contains(this->info_cache_, c)) {
        return info_cache_[c];
    }

    if(FT_Load_Glyph(face_, FT_Get_Char_Index(face_, c), FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT) != 0) {
        throw std::runtime_error("Unable to load glyph for character");
    }

    FT_Glyph glyph;
    if(FT_Get_Glyph(face_->glyph, &glyph) != 0) {
        throw std::runtime_error("Unable to load glyph for character");
    }

    FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);

    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) glyph;
    FT_Bitmap& bitmap = bitmap_glyph->bitmap;

    CharacterInfo new_info;
    new_info.advance_x = face_->glyph->metrics.horiAdvance >> 6;
    new_info.height = face_->glyph->metrics.height >> 6;
    new_info.width = face_->glyph->metrics.width >> 6;
    new_info.offset_x = face_->glyph->metrics.horiBearingX >> 6;
    new_info.offset_y = ((face_->glyph->metrics.horiBearingY - face_->glyph->metrics.height) >> 6);

    /*
     * Create a new texture and load in the bitmap data
     */
    Texture& tex = interface_.scene().texture(interface_.scene().new_texture());
    tex.resize(bitmap.width, bitmap.rows);

    std::vector<uint8_t>& data = tex.data();

    for(uint16_t j = 0; j < bitmap.rows; ++j) {
        for(uint16_t i = 0; i < bitmap.width; ++i) {
            uint16_t idx = (tex.bpp() / 8) * (i + (j * bitmap.width));
            uint16_t source_idx = (j * bitmap.width) + i;
            data[idx] = data[idx + 1] = data[idx + 2] = bitmap.buffer[source_idx];
            if(tex.bpp() == 32) {
                data[idx + 3] = data[idx];
            }
        }
    }

    /*
     * Get the current texture that is not full
     */
    Texture& dest = interface_.scene().texture(textures_[current_texture_]);

    uint16_t x_offset = row_next_left_;

    /*
     *  If this new glyph won't fit in the row, move to the next one.
     *  If there are no more, then create a new texture and make it the
     *  first glyph in that one
     */
    if(row_next_left_ + bitmap.width >= FONT_TEXTURE_SIZE) {
        current_texture_row_++;
        if(current_texture_row_ >= this->vertical_texture_rows_) {
            Texture& new_tex = interface_.scene().texture(interface_.scene().new_texture());
            new_tex.resize(FONT_TEXTURE_SIZE, FONT_TEXTURE_SIZE);

            textures_.push_back(
                new_tex.id()
            );
            current_texture_ = textures_.size() - 1;
            current_texture_row_ = 0;
        }
        row_next_left_ = 0;
        x_offset = 1;
    }

    // Blit the glyph to the texture
    uint16_t y_offset = current_texture_row_ * (face_->size->metrics.height);
    dest.sub_texture(tex.id(), x_offset, y_offset);
    dest.upload(false, false, false, /*linear=*/true);

    // Store the texture ID for this glyph
    new_info.texture = textures_[current_texture_];

    // Calculate the texture coordinates for the glyph
    kmVec2Fill(&new_info.texture_coordinates[0],
        float(x_offset) / float(FONT_TEXTURE_SIZE),
        float(y_offset + tex.height()) / float(FONT_TEXTURE_SIZE)
    );

    kmVec2Fill(&new_info.texture_coordinates[1],
        float(x_offset + tex.width()) / float(FONT_TEXTURE_SIZE),
        float(y_offset + tex.height()) / float(FONT_TEXTURE_SIZE)
    );

    kmVec2Fill(&new_info.texture_coordinates[2],
        float(x_offset + tex.width()) / float(FONT_TEXTURE_SIZE),
        float(y_offset) / float(FONT_TEXTURE_SIZE)
    );

    kmVec2Fill(&new_info.texture_coordinates[3],
        float(x_offset) / float(FONT_TEXTURE_SIZE),
        float(y_offset) / float(FONT_TEXTURE_SIZE)
    );
    row_next_left_ += new_info.width + 1;

    new_info.kern_x = std::bind(get_kerning_x, face_, c, std::tr1::placeholders::_1);

    //Cache this glyph
    info_cache_[c] = new_info;
    return info_cache_[c];
}

unicode Font::family_name() const {
    return std::string(face_->family_name);
}

unicode Font::style_name() const {
    return std::string(face_->style_name);
}

}
}
}


