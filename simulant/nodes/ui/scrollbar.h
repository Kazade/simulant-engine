#pragma once

#include "simulant/nodes/stage_node.h"
#include "simulant/utils/params.h"
#include "widget.h"

namespace smlt {
namespace ui {

enum ScrollbarOrientation {
    SCROLLBAR_ORIENTATION_HORIZONTAL,
    SCROLLBAR_ORIENTATION_VERTICAL
};

/** A scrollbar widget. The background layer is the track and the foreground
 *  layer is the thumb. Both can be styled by the caller in the usual way
 *  (including setting a background image on the track). */
class Scrollbar: public Widget {
public:
    S_DEFINE_STAGE_NODE_META("scrollbar");
    S_DEFINE_CORE_WIDGET_PROPERTIES(Scrollbar);

    using Widget::clean_up;
    using Widget::init;

    Scrollbar(Scene* owner);

    void set_orientation(ScrollbarOrientation orientation);
    ScrollbarOrientation orientation() const {
        return orientation_;
    }

    /** The total size of the scrollable content and the size of the viewport. */
    void set_scroll_range(Px content_extent, Px viewport_extent);

    Px content_extent() const {
        return content_extent_;
    }
    Px viewport_extent() const {
        return viewport_extent_;
    }

    void set_scroll_offset(Px offset);
    Px scroll_offset() const {
        return scroll_offset_;
    }

private:
    bool on_create(Params params) override;

    WidgetBounds
        calculate_foreground_size(const UIDim& content_dimensions) const override;

    ScrollbarOrientation orientation_ = SCROLLBAR_ORIENTATION_VERTICAL;
    Px content_extent_ = Px(0);
    Px viewport_extent_ = Px(0);
    Px scroll_offset_ = Px(0);
};

} // namespace ui
} // namespace smlt
