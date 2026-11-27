#pragma once

#include "simulant/nodes/stage_node.h"
#include "simulant/utils/params.h"
#include "widget.h"

#include <unordered_map>

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

enum ScrollDirection : uint8_t {
    SCROLL_DIRECTION_NONE = 0,
    SCROLL_DIRECTION_VERTICAL = 1 << 0,
    SCROLL_DIRECTION_HORIZONTAL = 1 << 1,
    SCROLL_DIRECTION_BOTH =
        SCROLL_DIRECTION_VERTICAL | SCROLL_DIRECTION_HORIZONTAL
};

inline ScrollDirection operator|(ScrollDirection lhs, ScrollDirection rhs) {
    return ScrollDirection(uint8_t(lhs) | uint8_t(rhs));
}

inline ScrollDirection operator&(ScrollDirection lhs, ScrollDirection rhs) {
    return ScrollDirection(uint8_t(lhs) & uint8_t(rhs));
}

class Scrollbar;

class Frame: public Widget, public RefCounted<Frame> {

    friend class FrameTests;

public:
    S_DEFINE_STAGE_NODE_META("frame");
    S_DEFINE_CORE_WIDGET_PROPERTIES(Frame);

    using Widget::clean_up;
    using Widget::init; // Pull in init to satisfy Managed<Image>

    Frame(Scene* owner);

    virtual ~Frame();

    bool on_create(Params params) override;

    bool pack_child(Widget* widget);
    bool unpack_child(Widget* widget,
                      ChildCleanup clean_up = CHILD_CLEANUP_DESTROY);

    const std::vector<smlt::ui::Widget*>& packed_children() const;

    void set_layout_direction(LayoutDirection dir);

    void set_space_between(Px spacing);

    Px space_between() const {
        return space_between_;
    }

    /** Enables scrolling in the given direction(s). A scrollbar is only
     *  shown when the frame's size in that direction is fixed. */
    void set_scroll_direction(ScrollDirection direction);
    ScrollDirection scroll_direction() const {
        return scroll_direction_;
    }

    void set_vertical_scrolling_enabled(bool enabled);
    void set_horizontal_scrolling_enabled(bool enabled);
    void set_scrolling_enabled(bool vertical, bool horizontal);
    void set_scrolling_enabled(bool enabled) {
        set_scrolling_enabled(enabled, enabled);
    }
    void set_scrolling_enabled(ScrollDirection direction) {
        set_scroll_direction(direction);
    }
    void enable_scrolling(ScrollDirection direction) {
        set_scroll_direction(direction);
    }

    bool vertical_scrolling_enabled() const;
    bool horizontal_scrolling_enabled() const;

    void set_scroll_offset(Px x, Px y);
    void set_scroll_x(Px x);
    void set_scroll_y(Px y);

    Px scroll_x() const {
        return scroll_x_;
    }
    Px scroll_y() const {
        return scroll_y_;
    }

    Px max_scroll_x() const;
    Px max_scroll_y() const;

    /** The local-space rectangle that packed children are clipped against.
     *  It excludes borders, padding, the title bar, and any visible
     *  scrollbars. */
    AABB clip_area() const {
        return clip_area_;
    }

    Scrollbar* horizontal_scrollbar() const {
        return horizontal_scrollbar_;
    }
    Scrollbar* vertical_scrollbar() const {
        return vertical_scrollbar_;
    }

    void set_scrollbar_width(Px width);

    /** Inset the scrollbars from the frame's inner edges. This does not
     *  affect the packed content's padding. */
    void set_scrollbar_inset(Px inset);
    void set_scrollbar_inset(Px left, Px right, Px bottom, Px top);
    UInt4 scrollbar_inset() const {
        return scrollbar_inset_;
    }

private:
    std::vector<smlt::ui::Widget*> children_;

    /* Tracks each packed child's destruction so its pointer is removed
     * before a later layout pass dereferences freed memory. */
    std::unordered_map<Widget*, sig::connection> child_destroy_connections_;
    void on_packed_child_destroyed(Widget* child);

    LayoutDirection direction_ = LAYOUT_DIRECTION_TOP_TO_BOTTOM;
    Px space_between_;

    ScrollDirection scroll_direction_ = SCROLL_DIRECTION_NONE;
    Px scroll_x_ = Px(0);
    Px scroll_y_ = Px(0);
    Px scrollbar_width_ = Px(16);
    UInt4 scrollbar_inset_ = {Px(), Px(), Px(), Px()};

    Scrollbar* horizontal_scrollbar_ = nullptr;
    Scrollbar* vertical_scrollbar_ = nullptr;

    AABB clip_area_;

    UIDim calculate_packed_content_dimensions() const;
    UIDim calculate_scroll_viewport() const;

    bool width_is_fixed() const;
    bool height_is_fixed() const;
    bool vertical_scrollbar_visible() const;
    bool horizontal_scrollbar_visible() const;

    void update_scrollbars(Px inner_left, Px inner_right, Px inner_bottom,
                           Px inner_top, Px content_left, Px content_right,
                           Px content_bottom, Px content_top);
    void clip_packed_children();

    virtual WidgetBounds calculate_foreground_size(
        const UIDim& content_dimensions) const override;
    virtual UIDim calculate_content_dimensions(Px text_width,
                                               Px text_height) override;

    virtual void finalize_build() override;

    /* A packed child resized itself after being packed (e.g. its text or
     * requested size changed) - re-run our layout to account for it, and
     * propagate further up if that changed our own size in turn. */
    void on_child_size_changed(Widget* child) override;

    /* Rebuilds, then notifies our own parent (if any) if that rebuild
     * changed our outer size - so nested frames re-flow correctly. */
    void rebuild_and_propagate_resize();
};

} // namespace ui

} // namespace smlt
