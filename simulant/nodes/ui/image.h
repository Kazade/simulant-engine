#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

struct ImageParams {
    UIConfig config;

    ImageParams(const UIConfig& config):
        config(config) {}
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
    Image(Scene *owner, UIConfig config);
    virtual ~Image() {}

    /* Set the texture of the Image. By default the image will be sized to the
     * full size of the texture, all set_source_rect to use a subsection of the texture */
    void set_texture(const TexturePtr& texture_id);

    /* Selects the source region of the texture to display in the image */
    void set_source_rect(const UICoord& bottom_left, const UICoord& size);

    bool set_resize_mode(ResizeMode resize_mode) override;
};

}

template<>
struct stage_node_traits<ui::Image> {
    typedef ui::ImageParams params_type;
    const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_IMAGE;
};


}
