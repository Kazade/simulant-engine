
#include "image.h"
#include "../../texture.h"

namespace smlt {
namespace ui {

Image::Image(WidgetID id, UIManager* owner, UIConfig* config):
    Widget(id, owner, config) {

    /* By default, images don't have a border */
    set_border_width(0);
    Widget::set_resize_mode(RESIZE_MODE_FIXED);
}

void Image::clear_layers() {
    set_text("");
}

void Image::set_texture_id(const TextureID &texture_id) {
    clear_layers();
    set_background_image(texture_id);

    // Changing the texture resets the source rect
    auto tex = texture_id.fetch();
    set_source_rect(Vec2(), tex->dimensions());
}

void Image::set_source_rect(const Vec2 &bottom_left, const Vec2 &size) {
    clear_layers();
    set_background_image_source_rect(bottom_left, size);

    /* Force the widget dimensions to match */
    set_width(size.x);
    set_height(size.y);
}

void Image::set_resize_mode(ResizeMode resize_mode) {
    L_WARN("Warning, tried to change the resize mode of an Image widget");
}

}
}
