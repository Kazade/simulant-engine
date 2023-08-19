#include "ui_manager.h"
#include "frame.h"
#include "../../meshes/mesh.h"

namespace smlt {
namespace ui {

Frame::Frame(Scene *owner, UIConfig config):
    Widget(owner, config, STAGE_NODE_TYPE_WIDGET_FRAME) {

    set_background_colour(config.frame_background_colour_);
    set_foreground_colour(config.frame_titlebar_colour_);
    set_text_colour(config.frame_text_colour_);
    set_border_width(config.frame_border_width_);
    set_border_colour(config.frame_border_colour_);
}

void Frame::finalize_build() {
    float ax = -(anchor_point().x - 0.5f);
    float ay = -(anchor_point().y - 0.5f);

    /* We reposition all the children relative to our centre */
    float cx = ax * outer_width().value;
    float cy = ay * outer_height().value;

    auto oh = outer_height() - (border_width() * 2);
    auto ow = outer_width() - (border_width() * 2);

    if(direction_ == LAYOUT_DIRECTION_LEFT_TO_RIGHT) {
        Px width = -(ow / 2);
        for(auto& child: packed_children()) {
            child->set_anchor_point(0.0f, 0.5f);
            child->move_to(cx + width.value, cy);
            width += child->outer_width();
            width += space_between_;
        }
    } else {
        Px height = (oh / 2);

        if(!text().empty()) {
            height -= line_height();
            height -= padding().top;
        }

        for(auto& child: packed_children()) {
            child->set_anchor_point(0.5f, 1.0f);
            child->move_to(cx, cy + height.value);
            height -= child->outer_height();
            height -= space_between_;
        }
    }

    /* Reposition the text to be in the title bar */
    if(!text_.empty()) {

        auto sm = mesh()->find_submesh("text");
        auto vdata = mesh()->vertex_data.get();

        Px line_height_shift = (text_height_ - font_->size()) / 2;
        line_height_shift += padding().top;

        Px shift = Px((oh.value * 0.5f) - line_height().value + (line_height_shift.value));

        for(std::size_t i = 0; i < sm->vertex_range_count(); ++i) {
            auto& range = sm->vertex_ranges()[i];
            for(auto idx = range.start; idx < range.start + range.count; ++idx) {
                auto vpos = *vdata->position_at<smlt::Vec3>(idx);
                vpos.y += shift.value;

                vdata->move_to(idx);
                vdata->position(vpos);
            }
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


Widget::WidgetBounds Frame::calculate_foreground_size(const UIDim& content_dimensions) const {
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
    _S_UNUSED(text_width);
    _S_UNUSED(text_height);

    Px content_width, content_height;

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

    if(direction_ == LAYOUT_DIRECTION_TOP_TO_BOTTOM) {
        content_height += (space_between() * int(children_.size() - 1));
    } else {
        content_width += (space_between() * int(children_.size() - 1));
    }

    /* Titlebar */
    if(!text().empty()) {
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

}
}
