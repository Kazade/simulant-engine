
#include "image.h"

namespace smlt {
namespace ui {

void Image::clear_layers() {
    set_background_image(TextureID());
    set_text("");
}

void Image::set_texture_id(const TextureID &texture_id) {
    clear_layers();
    set_foreground_image(texture_id);
}

void Image::set_source_rect(const Vec2 &bottom_left, const Vec2 &size) {
    clear_layers();
    set_foreground_source_rect(bottom_left, size);
}

}
}
