#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

struct ImageParams : public WidgetParams {
    TexturePtr texture;

    ImageParams(
        const TexturePtr& texture,
        const UIConfig& theme=UIConfig(),
        WidgetStylePtr shared_style=WidgetStylePtr()
    ):
        WidgetParams(theme, shared_style),
        texture(texture) {}
};

/* An Image widget, useful for health indicators etc.
 *
 * All images have their resize mode set to fixed width and do not
 * allow text.
 */
class Image:
    public Widget {

    void clear_layers();

public:
    struct Meta {
        typedef ui::ImageParams params_type;
        const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_IMAGE;
    };

    Image(Scene *owner);
    virtual ~Image() {}

    /* Set the texture of the Image. By default the image will be sized to the
     * full size of the texture, all set_source_rect to use a subsection of the texture */
    void set_texture(const TexturePtr& texture_id);

    /* Selects the source region of the texture to display in the image */
    void set_source_rect(const UICoord& bottom_left, const UICoord& size);

    bool set_resize_mode(ResizeMode resize_mode) override;

    bool on_create(void* params) override;
};

}
}
