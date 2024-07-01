#pragma once

#include "simulant/nodes/stage_node.h"
#include "simulant/utils/construction_args.h"
#include "widget.h"

namespace smlt {
namespace ui {

/* An Image widget, useful for health indicators etc.
 *
 * All images have their resize mode set to fixed width and do not
 * allow text.
 */
class Image: public Widget {

    void clear_layers();

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_IMAGE);
    S_DEFINE_STAGE_NODE_PARAM(Image, "texture", texture, no_value,
                              "The texture to display in the image");
    S_DEFINE_CORE_WIDGET_PROPERTIES(Image);

    Image(Scene* owner);
    virtual ~Image() {}

    /* Set the texture of the Image. By default the image will be sized to the
     * full size of the texture, all set_source_rect to use a subsection of the
     * texture */
    void set_texture(const TexturePtr& texture_id);

    /* Selects the source region of the texture to display in the image */
    void set_source_rect(const UICoord& bottom_left, const UICoord& size);

    bool set_resize_mode(ResizeMode resize_mode) override;

    bool on_create(Params params) override;
};

} // namespace ui
} // namespace smlt
