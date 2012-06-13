#include "font.h"
#include "kazbase/os/path.h"
#include "kazbase/exceptions.h"

namespace kglt {

Font::Font():
    kt_font_(0) {
    ktGenFonts(1, &kt_font_);
}

Font::~Font() {
    ktDeleteFonts(1, &kt_font_);
    kt_font_ = 0;
}

void Font::initialize(const std::string& ttf_path, const uint32_t font_size) {
    if(!os::path::exists(ttf_path)) {
        throw IOError("TTF file doesn't exist: " + ttf_path);
    }
    ktBindFont(kt_font_);
    ktLoadFont(ttf_path.c_str(), font_size);
    font_size_ = font_size;
}

double Font::string_width_in_pixels(const std::string& str) const {
    return ktStringWidthInPixels(str.c_str());
}

}
