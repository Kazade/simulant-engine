#include "kazbase/exceptions.h"
#include "font.h"

namespace kglt {
namespace extra {
namespace ui{

struct FreeTypeInitializer {
    /*
     *  Makes sure that FreeType is initialized and destroyed only once
     */
    FreeTypeInitializer() {
        FT_Init_FreeType(&ftlib);
    }

    ~FreeTypeInitializer() {
        FT_Done_FreeType(ftlib);
    }

    FT_Library ftlib;
};

static FreeTypeInitializer ft;

Font::Font(const std::string &path, uint8_t height):
    ttf_file_(path),
    font_height_(height),
    horizontal_glyph_count_(0),
    vertical_glyph_count_(0) {

    if(FT_New_Face(ft.ftlib, ttf_file_.c_str(), 0, &face_) != 0) {
        throw IOError("Unable to load font: " + path);
    }

    horizontal_glyph_count_ = FONT_TEXTURE_SIZE / face_->max_advance_width;
    vertical_glyph_count_ = FONT_TEXTURE_SIZE / face_->max_advance_height;
}

}
}
}


