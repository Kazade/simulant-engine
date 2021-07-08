
#include "image.h"
#include "../../texture.h"
#include "../../macros.h"

namespace smlt {
namespace ui {

Image::Image(UIManager* owner, UIConfig* config):
    Widget(owner, config) {

    /* By default, images don't have a border */
    set_border_width(0);
    set_border_colour(smlt::Colour::NONE);
    set_foreground_colour(smlt::Colour::NONE);

    if(!Widget::set_resize_mode(RESIZE_MODE_FIXED)) {
        // Rebuild if the resize mode didn't change
        rebuild();
    }
}

void Image::clear_layers() {
    set_text("");
}

void Image::set_texture(const TexturePtr &texture) {
    clear_layers();
    set_background_image(texture);

    auto dim = texture->dimensions();
    // Changing the texture resets the source rect
    set_source_rect(UICoord(), UICoord(dim.x, dim.y));
}

void Image::set_source_rect(const UICoord &bottom_left, const UICoord& size) {
    clear_layers();
    set_background_image_source_rect(bottom_left, size);

    /* Force the widget dimensions to match */
    resize(size.x, size.y);
}

bool Image::set_resize_mode(ResizeMode resize_mode) {
    _S_UNUSED(resize_mode);
    S_WARN("Warning, tried to change the resize mode of an Image widget");
    return false;
}

}
}
