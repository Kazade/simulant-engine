#include "ui_manager.h"
#include "frame.h"
#include "../../meshes/mesh.h"

namespace smlt {
namespace ui {

Frame::Frame(UIManager *owner, UIConfig *config):
    Widget(owner, config) {

    set_background_colour(config->frame_background_colour_);
    set_foreground_colour(config->frame_titlebar_colour_);
    set_text_colour(config->frame_text_colour_);
    set_border_width(config->frame_border_width_);
    set_border_colour(config->frame_border_colour_);
}

void Frame::finalize_build() {
    float ax = -(anchor_point().x - 0.5f);
    float ay = -(anchor_point().y - 0.5f);

    /* We reposition all the children relative to our centre */
    float cx = ax * outer_width().value;
    float cy = ay * outer_height().value;

    if(direction_ == LAYOUT_DIRECTION_LEFT_TO_RIGHT) {
        Px width = 0;
        for(auto& child: packed_children()) {
            child->move_to(cx + width.value, cy);
            width += child->outer_width();
            width += space_between_;
        }
    } else {
        Px height = (outer_height() / 2 ) - line_height().value;
        for(auto& child: packed_children()) {
            child->set_anchor_point(0.5f, 1.0f);

            /* FIXME! I don't know why subtracting the padding is necessary! */
            child->move_to(cx - padding().left.value, cy + height.value - padding().top.value);
            height -= child->outer_height();
            height -= space_between_;
        }
    }

    /* Reposition the text to be in the title bar */
    if(!pimpl_->text_.empty()) {

        auto sm = mesh()->find_submesh("text");
        auto vdata = mesh()->vertex_data.get();

        Px line_height_shift = (pimpl_->text_height_ - font_->size()) / 2;
        Px shift =  (int16_t) ((outer_height().value * 0.5f) - line_height().value + (line_height_shift.value));

        for(auto& idx: sm->index_data->all()) {
            auto vpos = *vdata->position_at<smlt::Vec3>(idx);
            vpos.y += shift.value;

            vdata->move_to(idx);
            vdata->position(vpos);
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

    Px content_width = 0;
    Px content_height = 0;

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

    auto p = padding();

    if(direction_ == LAYOUT_DIRECTION_TOP_TO_BOTTOM) {
        content_height += (space_between() * (children_.size() - 1));
    } else {
        content_width += (space_between() * (children_.size() - 1));
    }

    content_height += line_height().value;
    content_height += (p.top + p.bottom);
    content_width += (p.left + p.right);
    content_height += (border_width() * 2);
    content_width += (border_width() * 2);

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

    Px hw = (int16_t) std::ceil(float(content_width.value) * 0.5f);
    Px hh = (int16_t) std::ceil(float(content_height.value) * 0.5f);

    WidgetBounds bounds;
    bounds.min = UICoord(-hw, -hh);
    bounds.max = UICoord(hw, hh);

    return bounds;
}

Widget::WidgetBounds Frame::calculate_foreground_size() const {
    /* Foreground height is literally line-height, if there is text. The width
     * is the same as the background size */

    WidgetBounds fg_size = calculate_background_size();
    if(!text().empty()) {
        fg_size.min = fg_size.max;
        fg_size.min.y -= line_height().value;
        fg_size.min.x.value *= -1;
    } else {
        fg_size.min = fg_size.max;
    }

    return fg_size;
}

UIDim Frame::calculate_content_dimensions(Px text_width, Px text_height, WidgetBounds bg_size, WidgetBounds fg_size) {
    _S_UNUSED(text_width);
    _S_UNUSED(text_height);
    _S_UNUSED(fg_size);

    return UIDim(Px(bg_size.width()), Px(bg_size.height()));
}

}
}
