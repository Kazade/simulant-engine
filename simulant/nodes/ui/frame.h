#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

/** A Frame is a containing widget that can have a title
 *  and can have child widgets added to it. Child widgets
 *  will be positioned one-after-the-other in the direction
 *  specified.
 *
 *  The text and foreground layers appear as a title of the box,
 *  the background layer will extend to contain the widgets added.
*/

enum LayoutDirection {
    LAYOUT_DIRECTION_TOP_TO_BOTTOM,
    LAYOUT_DIRECTION_LEFT_TO_RIGHT
};

enum ChildCleanup {
    CHILD_CLEANUP_DESTROY,
    CHILD_CLEANUP_RETAIN
};

class Frame:
    public Widget {

public:
    using Widget::init; // Pull in init to satisfy Managed<Image>
    using Widget::clean_up;

    Frame(UIManager* owner, UIConfig* config, Stage* stage);

    bool pack_child(Widget* widget);

    bool unpack_child(Widget* widget, ChildCleanup clean_up=CHILD_CLEANUP_DESTROY);

    const std::vector<smlt::ui::Widget*>& packed_children() const;

    void set_layout_direction(LayoutDirection dir);

    void set_space_between(Px spacing);

    Px space_between() const {
        return space_between_;
    }

private:
    std::vector<smlt::ui::Widget*> children_;
    LayoutDirection direction_ = LAYOUT_DIRECTION_TOP_TO_BOTTOM;
    Px space_between_ = 0;

    virtual WidgetBounds calculate_foreground_size(const UIDim& content_dimensions) const override;
    virtual UIDim calculate_content_dimensions(Px text_width, Px text_height) override;

    virtual void finalize_build() override;
};


}
}
