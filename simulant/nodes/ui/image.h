#pragma once

#include "widget.h"

namespace smlt {
namespace ui {


/* An Image widget, useful for health indicators etc.
 *
 * All images have their resize mode set to fixed width and do not
 * allow text.
 */
class Image:
    public Widget,
    public Managed<Label> {

    void clear_layers();
public:
    /* Set the texture of the Image. By default the image will be sized to the
     * full size of the texture, all set_source_rect to use a subsection of the texture */
    void set_texture_id(const TextureID& texture_id);

    /* Selects the source region of the texture to display in the image */
    void set_source_rect(const Vec2& bottom_left, const Vec2& size);
};

}
}
