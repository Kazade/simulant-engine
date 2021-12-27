#include "ui_manager.h"
#include "frame.h"

namespace smlt {
namespace ui {

Frame::Frame(UIManager *owner, UIConfig *config):
    Widget(owner, config) {

}

void Frame::prepare_build() {
    /* We reposition all the children relative to our centre */

    if(direction_ == LAYOUT_DIRECTION_LEFT_TO_RIGHT) {

    } else {
        Px height = 0;
        for(auto& child: packed_children()) {
            height += child->outer_height();
        }

        height += (space_between() * (packed_children().size() - 1));

        Px y = (height / 2);
        for(auto& child: packed_children()) {
            auto ap = child->anchor_point();
            child->set_anchor_point(0.5f, 1.0f);

            child->move_to(0.0f, y.value);
            y -= child->outer_height();
            y -= space_between();

            // Restore anchor point
            child->set_anchor_point(ap.x, ap.y);
        }
    }
}

bool Frame::pack_child(Widget *widget) {
    if(widget == this) {
        return false;
    }

    auto it = std::find(children_.begin(), children_.end(), widget);
    if(it == children_.end()) {
        widget->set_parent(this);  // Reparent
        children_.push_back(widget);
        rebuild();
        return true;
    }

    return false;
}

bool Frame::unpack_child(Widget *widget, ChildCleanup clean_up) {
    auto it = std::find(children_.begin(), children_.end(), widget);
    if(it != children_.end()) {
        children_.erase(std::remove(children_.begin(), children_.end(), widget));
        if(clean_up == CHILD_CLEANUP_DESTROY) {
            widget->destroy();
        } else {
            widget->set_parent(nullptr);
        }
        rebuild();
        return true;
    }

    return false;
}

const std::vector<Widget *> &Frame::packed_children() const {
    return children_;
}

void Frame::set_layout_direction(LayoutDirection dir) {
    if(direction_ == dir) {
        return;
    }

    direction_ = dir;
    rebuild();
}

void Frame::set_space_between(Px spacing) {
    if(space_between_ == spacing) {
        return;
    }

    space_between_ = spacing;
    rebuild();
}

Widget::WidgetBounds Frame::calculate_background_size() const {
    auto mode = resize_mode();

    Px content_width = 0, content_height = 0;

    for(auto& child: packed_children()) {
        if(direction_ == LAYOUT_DIRECTION_TOP_TO_BOTTOM) {
            content_width = std::max(content_width, child->outer_width());
            content_height += child->outer_height();
        } else {
            content_width += child->outer_width();
            content_height = std::max(content_height, child->outer_height());
        }
    }

    auto p = padding();

    content_height += (p.top + p.bottom);
    content_width += (p.left + p.right);

    if(direction_ == LAYOUT_DIRECTION_TOP_TO_BOTTOM) {
        content_height += (space_between() * (children_.size() - 1));
    } else {
        content_width += (space_between() * (children_.size() - 1));
    }

    if(mode == RESIZE_MODE_FIXED) {
        content_width = p.left + requested_width() + p.right;
        content_height = p.top + requested_height() + p.bottom;
    } else if(mode == RESIZE_MODE_FIXED_HEIGHT) {
        content_height = p.top + requested_height() + p.bottom;
    } else if(mode == RESIZE_MODE_FIXED_WIDTH) {
        content_width = p.left + requested_width() + p.right;
    } else {
        /* Do nothing, all dynamic */
    }

    auto hw = std::ceil(float(content_width.value) * 0.5f);
    auto hh = std::ceil(float(content_height.value) * 0.5f);

    WidgetBounds bounds;
    bounds.min = smlt::Vec2(-hw, -hh);
    bounds.max = smlt::Vec2(hw, hh);

    return bounds;
}

Widget::WidgetBounds Frame::calculate_foreground_size() const {
    /* Foreground height is literally line-height, if there is text. The width
     * is the same as the background size */

    WidgetBounds fg_size = calculate_background_size();
    if(!text().empty()) {
        fg_size.min = fg_size.max - smlt::Vec2(0, line_height().value);
    } else {
        fg_size.min = fg_size.max;
    }

    return fg_size;
}

std::pair<Px, Px> Frame::calculate_content_dimensions(float text_width, float text_height, WidgetBounds bg_size, WidgetBounds fg_size) {
    _S_UNUSED(text_width);
    _S_UNUSED(text_height);
    _S_UNUSED(fg_size);

    return std::make_pair(Px(bg_size.width()), Px(bg_size.height()));
}

}
}
