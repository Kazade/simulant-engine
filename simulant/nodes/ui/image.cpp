#include "image.h"
#include "../../macros.h"
#include "../../texture.h"
#include "simulant/types.h"
#include "simulant/utils/construction_args.h"
#include "ui_manager.h"

namespace smlt {
namespace ui {

Image::Image(Scene* owner) :
    Widget(owner, STAGE_NODE_TYPE_WIDGET_IMAGE) {}

bool Image::on_create(const Params& params) {
    if(!Widget::on_create(params)) {
        return false;
    }

    auto sstyle = params.arg<WidgetStyle>("shared_style");
    auto theme = params.arg<UIConfig>("theme").value_or(UIConfig());
    auto texture = params.arg<TexturePtr>("texture").value_or(TexturePtr());
    if(!sstyle) {
        /* By default, images don't have a border */
        set_border_width(theme.image_border_width_);
        set_border_color(smlt::Color::NONE);
        set_background_color(theme.image_background_color_);
        set_padding(theme.image_padding_.left, theme.image_padding_.right,
                    theme.image_padding_.bottom, theme.image_padding_.top);

        set_foreground_color(theme.image_foreground_color_);

        if(texture) {
            set_texture(texture);
        }

        if(!Widget::set_resize_mode(RESIZE_MODE_FIXED)) {
            // Rebuild if the resize mode didn't change
            rebuild();
        }
    }

    return true;
}

void Image::clear_layers() {
    set_text("");
}

void Image::set_texture(const TexturePtr& texture) {
    clear_layers();
    set_background_image(texture);

    auto dim = texture->dimensions();
    // Changing the texture resets the source rect
    set_source_rect(UICoord(), UICoord(Px(dim.x), Px(dim.y)));
}

void Image::set_source_rect(const UICoord& bottom_left, const UICoord& size) {
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

} // namespace ui
} // namespace smlt
