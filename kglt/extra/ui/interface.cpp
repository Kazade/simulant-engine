#include "interface.h"
#include "font.h"

namespace kglt {
namespace extra {
namespace ui {

Interface::Interface(Scene &scene, uint32_t width_in_pixels, uint32_t height_in_pixels):
    scene_(scene),
    width_(width_in_pixels),
    height_(height_in_pixels) {

}

void Interface::load_font(const std::string &ttf_file, uint8_t font_height) {
    Font::ptr font = Font::create(ttf_file, font_height);
}

}
}
}
