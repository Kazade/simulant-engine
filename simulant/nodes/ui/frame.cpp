#include "frame.h"
#include "../../meshes/mesh.h"
#include "../../scenes/scene.h"
#include "scrollbar.h"
#include "simulant/utils/params.h"
#include "ui_manager.h"

#include <algorithm>

namespace smlt {
namespace ui {

Frame::Frame(Scene* owner) :
    Widget(owner, Meta::node_type) {}

Frame::~Frame() {
    for(auto& p: child_destroy_connections_) {
        p.second.disconnect();
    }
}

void Frame::on_packed_child_destroyed(Widget* child) {
    auto it = child_destroy_connections_.find(child);
    if(it != child_destroy_connections_.end()) {
        it->second.disconnect();
        child_destroy_connections_.erase(it);
    }

    auto cit = std::find(children_.begin(), children_.end(), child);
    if(cit != children_.end()) {
        children_.erase(cit);
    }
}

bool Frame::on_create(Params params) {
    if(!clean_params<Frame>(params)) {
        return false;
    }

    if(!Widget::on_create(params)) {
        return false;
    }

    auto sstyle = params.get<WidgetStyleRef>("shared_style")
                      .value_or(WidgetStyleRef())
                      .lock();
    auto theme = params.get<UIConfig>("theme").value_or(UIConfig());

    if(!sstyle) {
        set_background_color(theme.frame_background_color_);
        set_foreground_color(theme.frame_titlebar_color_);
        set_text_color(theme.frame_text_color_);
        set_border_width(theme.frame_border_width_);
        set_border_color(theme.frame_border_color_);
    }

    scrollbar_width_ = Px(theme.scrollbar_width_);
    if(scrollbar_width_.value <= 0) {
        scrollbar_width_ = Px(16);
    }

    /* Scrollbars are ordinary child widgets so callers can style them (or
     * replace their background image) directly. They are positioned by
     * finalize_build() and aren't part of the packed child layout. */
    vertical_scrollbar_ = create_child<Scrollbar>("vertical_scrollbar");
    if(vertical_scrollbar_) {
        vertical_scrollbar_->set_orientation(
            SCROLLBAR_ORIENTATION_VERTICAL);
        vertical_scrollbar_->set_background_color(
            theme.scrollbar_background_color_);
        vertical_scrollbar_->set_foreground_color(
            theme.scrollbar_foreground_color_);
        vertical_scrollbar_->set_visible(false);
    }

    horizontal_scrollbar_ = create_child<Scrollbar>("horizontal_scrollbar");
    if(horizontal_scrollbar_) {
        horizontal_scrollbar_->set_orientation(
            SCROLLBAR_ORIENTATION_HORIZONTAL);
        horizontal_scrollbar_->set_background_color(
            theme.scrollbar_background_color_);
        horizontal_scrollbar_->set_foreground_color(
            theme.scrollbar_foreground_color_);
        horizontal_scrollbar_->set_visible(false);
    }

    auto width = params.get<int>("width").value_or(-1);
    auto height = params.get<int>("height").value_or(-1);
    if(width > -1 || height > -1) {
        resize(width, height);
    }

    return true;
}

void Frame::finalize_build() {
    float ax = -(anchor_point().x - 0.5f);
    float ay = -(anchor_point().y - 0.5f);

    /* We reposition all the children relative to our center */
    float cx = ax * outer_width().value;
    float cy = ay * outer_height().value;

    auto oh = outer_height() - (border_width() * 2);
    auto ow = outer_width() - (border_width() * 2);

    Px content_left = Px(cx) - (ow / 2) + padding().left;
    Px content_right = Px(cx) + (ow / 2) - padding().right;
    Px content_bottom = Px(cy) - (oh / 2) + padding().bottom;
    Px content_top = Px(cy) + (oh / 2) - padding().top;

    /* The frame's inner bounds (inside the border). Scrollbars sit against
     * these edges so padding only affects the packed content, not the
     * scrollbars themselves. */
    Px inner_left = Px(cx) - (ow / 2);
    Px inner_right = Px(cx) + (ow / 2);
    Px inner_bottom = Px(cy) - (oh / 2);
    Px inner_top = Px(cy) + (oh / 2);

    if(!text().empty()) {
        content_top -= line_height();
    }

    /* Scrollbars live inside the frame bounds, so take their space out of
     * the content area before laying out (and clipping) the packed children. */
    if(vertical_scrollbar_visible()) {
        content_right -= (scrollbar_width_ + scrollbar_inset_.right);
    }

    if(horizontal_scrollbar_visible()) {
        content_bottom += (scrollbar_width_ + scrollbar_inset_.bottom);
    }

    auto packed = calculate_packed_content_dimensions();
    Px viewport_width = std::max(Px(0), content_right - content_left);
    Px viewport_height = std::max(Px(0), content_top - content_bottom);

    /* Centre non-scrolling-axis children within the content viewport, not
     * the whole frame. Otherwise a scrollbar can reduce the viewport on one
     * side and a centred child will still creep underneath it. */
    Px viewport_center_x = content_left + (viewport_width / 2);
    Px viewport_center_y = content_bottom + (viewport_height / 2);

    Px max_x = (horizontal_scrolling_enabled() && width_is_fixed())
                   ? std::max(Px(0), packed.width - viewport_width)
                   : Px(0);
    Px max_y = (vertical_scrolling_enabled() && height_is_fixed())
                   ? std::max(Px(0), packed.height - viewport_height)
                   : Px(0);

    scroll_x_ = std::max(Px(0), std::min(scroll_x_, max_x));
    scroll_y_ = std::max(Px(0), std::min(scroll_y_, max_y));

    if(direction_ == LAYOUT_DIRECTION_LEFT_TO_RIGHT) {
        Px x = content_left - scroll_x_;

        if(vertical_scrolling_enabled()) {
            /* Left-align to the top of the viewport when we may scroll
             * vertically, so the scroll range matches the content size. */
            Px top = content_top - scroll_y_;
            for(auto& child: packed_children()) {
                child->set_anchor_point(0.0f, 1.0f);
                child->transform->set_translation_2d(
                    Vec2(x.value, top.value));
                x += child->outer_width();
                x += space_between_;
            }
        } else {
            for(auto& child: packed_children()) {
                child->set_anchor_point(0.0f, 0.5f);
                child->transform->set_translation_2d(
                    Vec2(x.value, (viewport_center_y - scroll_y_).value));
                x += child->outer_width();
                x += space_between_;
            }
        }
    } else {
        Px y = content_top - scroll_y_;

        if(horizontal_scrolling_enabled()) {
            /* Top-align to the left of the viewport when we may scroll
             * horizontally, so the scroll range matches the content size. */
            Px left = content_left - scroll_x_;
            for(auto& child: packed_children()) {
                child->set_anchor_point(0.0f, 1.0f);
                child->transform->set_translation_2d(
                    Vec2(left.value, y.value));
                y -= child->outer_height();
                y -= space_between_;
            }
        } else {
            for(auto& child: packed_children()) {
                child->set_anchor_point(0.5f, 1.0f);
                child->transform->set_translation_2d(
                    Vec2((viewport_center_x - scroll_x_).value, y.value));
                y -= child->outer_height();
                y -= space_between_;
            }
        }
    }

    clip_area_.set_min_max(Vec3(content_left.value, content_bottom.value, 0),
                           Vec3(content_right.value, content_top.value, 0));

    update_scrollbars(inner_left, inner_right, inner_bottom, inner_top,
                      content_left, content_right, content_bottom,
                      content_top);
    clip_packed_children();

    /* Reposition the text to be in the title bar */
    if(!text_.empty()) {
        static_assert(Font::max_pages == 4, "This code needs changing");
        SubMeshPtr submeshes[] = {
            mesh()->find_submesh("text-0"),
            mesh()->find_submesh("text-1"),
            mesh()->find_submesh("text-2"),
            mesh()->find_submesh("text-3"),
        };

        auto vdata = mesh()->vertex_data.get();

        /* The titlebar sits flush against the top of the frame (padding()
         * only applies between the titlebar and the first child, handled
         * above), so the text should be centred within a line_height()-tall
         * strip starting at the very top of the box. */
        Px baseline_shift = (text_height_ - font_->size()) / 2;
        Px shift = Px((oh.value * 0.5f) - (line_height().value * 0.5f) +
                      baseline_shift.value);

        for(auto& sm: submeshes) {
            if(!sm) {
                continue;
            }

            for(std::size_t i = 0; i < sm->vertex_range_count(); ++i) {
                auto& range = sm->vertex_ranges()[i];
                for(auto idx = range.start; idx < range.start + range.count;
                    ++idx) {
                    auto vpos = *vdata->position_at<smlt::Vec3>(idx);
                    vpos.y += shift.value;

                    vdata->move_to(idx);
                    vdata->position(vpos);
                }
            }
        }
    }
}

bool Frame::pack_child(Widget* widget) {
    if(widget == this) {
        return false;
    }

    auto it = std::find(children_.begin(), children_.end(), widget);
    if(it == children_.end()) {
        widget->set_parent(this); // Reparent
        children_.push_back(widget);
        // Drop our pointer if the child is destroyed while packed.
        child_destroy_connections_[widget] =
            widget->signal_destroyed().connect([this, widget]() {
            on_packed_child_destroyed(widget);
        });
        rebuild_and_propagate_resize();
        return true;
    }

    return false;
}

bool Frame::unpack_child(Widget* widget, ChildCleanup clean_up) {
    auto it = std::find(children_.begin(), children_.end(), widget);
    if(it != children_.end()) {
        children_.erase(
            std::remove(children_.begin(), children_.end(), widget));

        auto cit = child_destroy_connections_.find(widget);
        if(cit != child_destroy_connections_.end()) {
            cit->second.disconnect();
            child_destroy_connections_.erase(cit);
        }

        if(clean_up == CHILD_CLEANUP_DESTROY) {
            widget->destroy();
        } else {
            widget->set_parent(nullptr);
        }
        rebuild_and_propagate_resize();
        return true;
    }

    return false;
}

const std::vector<Widget*>& Frame::packed_children() const {
    return children_;
}

void Frame::on_child_size_changed(Widget* child) {
    /* Scrollbars are children but not packed, so resizing them while we are
     * laying out must not trigger another layout pass. */
    if(std::find(children_.begin(), children_.end(), child) ==
       children_.end()) {
        return;
    }

    rebuild_and_propagate_resize();
}

void Frame::set_layout_direction(LayoutDirection dir) {
    if(direction_ == dir) {
        return;
    }

    direction_ = dir;
    rebuild_and_propagate_resize();
}

void Frame::set_space_between(Px spacing) {
    if(space_between_ == spacing) {
        return;
    }

    space_between_ = spacing;
    rebuild_and_propagate_resize();
}

void Frame::set_scroll_direction(ScrollDirection direction) {
    if(scroll_direction_ == direction) {
        return;
    }

    scroll_direction_ = direction;
    rebuild();
}

void Frame::set_vertical_scrolling_enabled(bool enabled) {
    auto direction = scroll_direction_;
    if(enabled) {
        direction = direction | SCROLL_DIRECTION_VERTICAL;
    } else {
        direction = ScrollDirection(
            uint8_t(direction) &
            uint8_t(~uint8_t(SCROLL_DIRECTION_VERTICAL)));
    }

    set_scroll_direction(direction);
}

void Frame::set_horizontal_scrolling_enabled(bool enabled) {
    auto direction = scroll_direction_;
    if(enabled) {
        direction = direction | SCROLL_DIRECTION_HORIZONTAL;
    } else {
        direction = ScrollDirection(
            uint8_t(direction) &
            uint8_t(~uint8_t(SCROLL_DIRECTION_HORIZONTAL)));
    }

    set_scroll_direction(direction);
}

void Frame::set_scrolling_enabled(bool vertical, bool horizontal) {
    ScrollDirection direction = SCROLL_DIRECTION_NONE;
    if(vertical) {
        direction = direction | SCROLL_DIRECTION_VERTICAL;
    }
    if(horizontal) {
        direction = direction | SCROLL_DIRECTION_HORIZONTAL;
    }

    set_scroll_direction(direction);
}

bool Frame::vertical_scrolling_enabled() const {
    return (scroll_direction_ & SCROLL_DIRECTION_VERTICAL) !=
           SCROLL_DIRECTION_NONE;
}

bool Frame::horizontal_scrolling_enabled() const {
    return (scroll_direction_ & SCROLL_DIRECTION_HORIZONTAL) !=
           SCROLL_DIRECTION_NONE;
}

void Frame::set_scroll_offset(Px x, Px y) {
    set_scroll_x(x);
    set_scroll_y(y);
}

void Frame::set_scroll_x(Px x) {
    x = std::max(Px(0), std::min(x, max_scroll_x()));
    if(scroll_x_ == x) {
        return;
    }

    scroll_x_ = x;
    rebuild();
}

void Frame::set_scroll_y(Px y) {
    y = std::max(Px(0), std::min(y, max_scroll_y()));
    if(scroll_y_ == y) {
        return;
    }

    scroll_y_ = y;
    rebuild();
}

void Frame::set_scrollbar_width(Px width) {
    if(width.value <= 0 || scrollbar_width_ == width) {
        return;
    }

    scrollbar_width_ = width;
    rebuild();
}

void Frame::set_scrollbar_inset(Px left, Px right, Px bottom, Px top) {
    if(scrollbar_inset_.left == left && scrollbar_inset_.right == right &&
       scrollbar_inset_.bottom == bottom && scrollbar_inset_.top == top) {
        return;
    }

    scrollbar_inset_ = UInt4{left, right, bottom, top};
    rebuild();
}

void Frame::set_scrollbar_inset(Px inset) {
    set_scrollbar_inset(inset, inset, inset, inset);
}

bool Frame::width_is_fixed() const {
    return resize_mode() == RESIZE_MODE_FIXED ||
           resize_mode() == RESIZE_MODE_FIXED_WIDTH;
}

bool Frame::height_is_fixed() const {
    return resize_mode() == RESIZE_MODE_FIXED ||
           resize_mode() == RESIZE_MODE_FIXED_HEIGHT;
}

bool Frame::vertical_scrollbar_visible() const {
    return vertical_scrolling_enabled() && height_is_fixed();
}

bool Frame::horizontal_scrollbar_visible() const {
    return horizontal_scrolling_enabled() && width_is_fixed();
}

UIDim Frame::calculate_packed_content_dimensions() const {
    Px content_width = Px(0);
    Px content_height = Px(0);

    for(auto& child: packed_children()) {
        auto child_width = child->outer_width();
        auto child_height = child->outer_height();

        if(direction_ == LAYOUT_DIRECTION_TOP_TO_BOTTOM) {
            content_width = std::max(content_width, child_width);
            content_height += child_height;
        } else {
            content_width += child_width;
            content_height = std::max(content_height, child_height);
        }
    }

    int gap_count = children_.empty() ? 0 : int(children_.size() - 1);
    if(direction_ == LAYOUT_DIRECTION_TOP_TO_BOTTOM) {
        content_height += (space_between() * gap_count);
    } else {
        content_width += (space_between() * gap_count);
    }

    return UIDim(content_width, content_height);
}

UIDim Frame::calculate_scroll_viewport() const {
    auto ow = outer_width() - (border_width() * 2);
    auto oh = outer_height() - (border_width() * 2);

    Px width = ow - padding().left - padding().right;
    Px height = oh - padding().top - padding().bottom;

    if(!text().empty()) {
        height -= line_height();
    }

    if(vertical_scrollbar_visible()) {
        width -= scrollbar_width_;
    }

    if(horizontal_scrollbar_visible()) {
        height -= scrollbar_width_;
    }

    return UIDim(std::max(Px(0), width), std::max(Px(0), height));
}

Px Frame::max_scroll_x() const {
    if(!horizontal_scrolling_enabled() || !width_is_fixed()) {
        return Px(0);
    }

    auto packed = calculate_packed_content_dimensions();
    auto viewport = calculate_scroll_viewport();
    return std::max(Px(0), packed.width - viewport.width);
}

Px Frame::max_scroll_y() const {
    if(!vertical_scrolling_enabled() || !height_is_fixed()) {
        return Px(0);
    }

    auto packed = calculate_packed_content_dimensions();
    auto viewport = calculate_scroll_viewport();
    return std::max(Px(0), packed.height - viewport.height);
}

void Frame::update_scrollbars(Px inner_left, Px inner_right, Px inner_bottom,
                              Px inner_top, Px content_left, Px content_right,
                              Px content_bottom, Px content_top) {
    auto packed = calculate_packed_content_dimensions();
    Px bar_width = scrollbar_width_;

    Px viewport_width = std::max(Px(0), content_right - content_left);
    Px viewport_height = std::max(Px(0), content_top - content_bottom);

    Px bar_top = inner_top;
    if(!text().empty()) {
        bar_top -= line_height();
    }

    if(vertical_scrollbar_) {
        bool visible = vertical_scrollbar_visible();
        vertical_scrollbar_->set_visible(visible);

        if(visible) {
            Px bar_height = std::max(Px(0), bar_top - inner_bottom);
            bar_height -= (scrollbar_inset_.bottom + scrollbar_inset_.top);
            if(horizontal_scrollbar_visible()) {
                bar_height = std::max(Px(0), bar_height - bar_width);
            } else {
                bar_height = std::max(Px(0), bar_height);
            }

            vertical_scrollbar_->set_anchor_point(0.0f, 0.0f);
            vertical_scrollbar_->set_resize_mode(RESIZE_MODE_FIXED);
            vertical_scrollbar_->resize(bar_width, bar_height);
            /* Pin the scrollbar to the frame's inner right edge, inset by
             * scrollbar_inset_. Padding is deliberately not included. */
            vertical_scrollbar_->transform->set_translation_2d(
                Vec2((inner_right - scrollbar_inset_.right - bar_width).value,
                     (inner_bottom + scrollbar_inset_.bottom).value));
            vertical_scrollbar_->set_scroll_range(packed.height,
                                                  viewport_height);
            vertical_scrollbar_->set_scroll_offset(scroll_y_);
        }
    }

    if(horizontal_scrollbar_) {
        bool visible = horizontal_scrollbar_visible();
        horizontal_scrollbar_->set_visible(visible);

        if(visible) {
            Px bar_length = std::max(Px(0), inner_right - inner_left);
            bar_length -= (scrollbar_inset_.left + scrollbar_inset_.right);
            if(vertical_scrollbar_visible()) {
                bar_length = std::max(Px(0), bar_length - bar_width);
            } else {
                bar_length = std::max(Px(0), bar_length);
            }

            horizontal_scrollbar_->set_anchor_point(0.0f, 0.0f);
            horizontal_scrollbar_->set_resize_mode(RESIZE_MODE_FIXED);
            horizontal_scrollbar_->resize(bar_length, bar_width);
            horizontal_scrollbar_->transform->set_translation_2d(
                Vec2((inner_left + scrollbar_inset_.left).value,
                     (inner_bottom + scrollbar_inset_.bottom).value));
            horizontal_scrollbar_->set_scroll_range(packed.width,
                                                    viewport_width);
            horizontal_scrollbar_->set_scroll_offset(scroll_x_);
        }
    }
}

void Frame::clip_packed_children() {
    auto clip_min = clip_area_.min();
    auto clip_max = clip_area_.max();

    /* If this frame is itself inside another clipped frame, intersect the
     * inherited clip with our own content clip. */
    if(clip_rect_) {
        auto inherited_min = clip_rect_->min();
        auto inherited_max = clip_rect_->max();

        clip_min.x = std::max(clip_min.x, inherited_min.x);
        clip_min.y = std::max(clip_min.y, inherited_min.y);
        clip_max.x = std::min(clip_max.x, inherited_max.x);
        clip_max.y = std::min(clip_max.y, inherited_max.y);
    }

    bool clip_has_area = clip_min.x < clip_max.x && clip_min.y < clip_max.y;

    for(auto& child: packed_children()) {
        auto pos = child->transform->translation_2d();
        auto anchor = child->anchor_point();

        /* Calculate the child's local bounds from its layout size rather
         * than its mesh AABB: widgets such as Label may have no background
         * layer (and therefore a mesh that only covers their text). */
        auto width = float(child->outer_width().value);
        auto height = float(child->outer_height().value);

        float child_left = pos.x - (anchor.x * width);
        float child_right = pos.x + ((1.0f - anchor.x) * width);
        float child_bottom = pos.y - (anchor.y * height);
        float child_top = pos.y + ((1.0f - anchor.y) * height);

        bool intersects = clip_has_area && child_right > clip_min.x &&
                          child_left < clip_max.x && child_top > clip_min.y &&
                          child_bottom < clip_max.y;

        child->set_visible(intersects);

        if(!intersects) {
            child->set_clip_rect(smlt::no_value);
            continue;
        }

        /* The child's mesh is in its own anchored local space; translate
         * the parent-space clip rect into it. */
        AABB child_clip;
        child_clip.set_min_max(
            Vec3(clip_min.x - pos.x, clip_min.y - pos.y, 0.0f),
            Vec3(clip_max.x - pos.x, clip_max.y - pos.y, 0.0f));
        child->set_clip_rect(child_clip);
    }
}

void Frame::rebuild_and_propagate_resize() {
    auto prev_width = outer_width();
    auto prev_height = outer_height();

    rebuild();

    /* If re-laying out our children changed our own size, tell our parent
     * too, so nested frames re-flow correctly. */
    if(outer_width() != prev_width || outer_height() != prev_height) {
        notify_parent_of_size_change();
    }
}

Widget::WidgetBounds
    Frame::calculate_foreground_size(const UIDim& content_dimensions) const {
    /* Foreground height is literally line-height, if there is text. The width
     * is the same as the background size */

    WidgetBounds fg_size = calculate_background_size(content_dimensions);
    if(!text().empty()) {
        fg_size.min = fg_size.max;
        fg_size.min.y -= line_height();
        fg_size.min.x.value *= -1;
    } else {
        fg_size.min = UICoord();
        fg_size.max = UICoord();
    }

    return fg_size;
}

UIDim Frame::calculate_content_dimensions(Px text_width, Px text_height) {
    _S_UNUSED(text_height);

    auto packed = calculate_packed_content_dimensions();
    Px content_width = packed.width;
    Px content_height = packed.height;

    /* Reserve space for scrollbars on fit-content axes. Fixed axes already
     * have their final size; the bar sits inside them and the viewport is
     * reduced in finalize_build() instead. */
    if(vertical_scrollbar_visible() && !width_is_fixed()) {
        content_width += (scrollbar_width_ + scrollbar_inset_.right);
    }

    if(horizontal_scrollbar_visible() && !height_is_fixed()) {
        content_height += (scrollbar_width_ + scrollbar_inset_.bottom);
    }

    /* Titlebar: reserve enough width for the title text too, since it isn't
     * one of the packed children and so wouldn't otherwise be considered. */
    if(!text().empty()) {
        content_width = std::max(content_width, text_width);
        content_height += line_height();
    }

    auto mode = resize_mode();

    if(mode == RESIZE_MODE_FIXED) {
        content_width = requested_width();
        content_height = requested_height();
    } else if(mode == RESIZE_MODE_FIXED_HEIGHT) {
        content_height = requested_height();
    } else if(mode == RESIZE_MODE_FIXED_WIDTH) {
        content_width = requested_width();
    } else {
        /* Do nothing, all dynamic */
    }

    return UIDim(content_width, content_height);
}

} // namespace ui
} // namespace smlt
