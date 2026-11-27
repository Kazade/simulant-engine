#include "scrollbar.h"

#include <algorithm>

#include "simulant/utils/params.h"
#include "ui_manager.h"

namespace smlt {
namespace ui {

Scrollbar::Scrollbar(Scene* owner) :
    Widget(owner, Meta::node_type) {}

bool Scrollbar::on_create(Params params) {
    if(!clean_params<Scrollbar>(params)) {
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
        set_background_color(theme.scrollbar_background_color_);
        set_foreground_color(theme.scrollbar_foreground_color_);
        set_border_width(Px(0));
        set_border_color(Color::none());
        set_padding(Px(0));
    }

    set_resize_mode(RESIZE_MODE_FIXED);

    auto width = params.get<int>("width").value_or(-1);
    auto height = params.get<int>("height").value_or(-1);
    if(width > -1 || height > -1) {
        resize(width, height);
    }

    return true;
}

void Scrollbar::set_orientation(ScrollbarOrientation orientation) {
    if(orientation_ == orientation) {
        return;
    }

    orientation_ = orientation;
    rebuild();
}

void Scrollbar::set_scroll_range(Px content_extent, Px viewport_extent) {
    content_extent_ = std::max(Px(0), content_extent);
    viewport_extent_ = std::max(Px(0), viewport_extent);

    auto max_offset =
        std::max(Px(0), content_extent_ - viewport_extent_);
    if(scroll_offset_ > max_offset) {
        scroll_offset_ = max_offset;
    }

    rebuild();
}

void Scrollbar::set_scroll_offset(Px offset) {
    auto max_offset = std::max(Px(0), content_extent_ - viewport_extent_);
    offset = std::max(Px(0), std::min(offset, max_offset));

    if(scroll_offset_ != offset) {
        scroll_offset_ = offset;
        rebuild();
    }
}

Widget::WidgetBounds
    Scrollbar::calculate_foreground_size(const UIDim& content_dimensions) const {
    auto bounds = calculate_background_size(content_dimensions);

    auto track_width = bounds.width().value;
    auto track_height = bounds.height().value;

    auto content_extent = content_extent_.value;
    auto viewport_extent = viewport_extent_.value;

    float fraction = 1.0f;
    if(content_extent > 0 && viewport_extent > 0 &&
       content_extent > viewport_extent) {
        fraction = float(viewport_extent) / float(content_extent);
        fraction = std::max(0.05f, std::min(fraction, 1.0f));
    }

    auto max_offset = std::max(0, content_extent - viewport_extent);
    float progress = 0.0f;
    if(max_offset > 0) {
        progress = float(scroll_offset_.value) / float(max_offset);
        progress = std::max(0.0f, std::min(progress, 1.0f));
    }

    if(orientation_ == SCROLLBAR_ORIENTATION_VERTICAL) {
        auto thumb_height = std::max(1, int(track_height * fraction));
        auto travel = std::max(0, track_height - thumb_height);
        auto top = bounds.max.y.value - int(travel * progress);

        WidgetBounds ret;
        ret.min = UICoord(bounds.min.x, Px(top - thumb_height));
        ret.max = UICoord(bounds.max.x, Px(top));
        return ret;
    } else {
        auto thumb_width = std::max(1, int(track_width * fraction));
        auto travel = std::max(0, track_width - thumb_width);
        auto left = bounds.min.x.value + int(travel * progress);

        WidgetBounds ret;
        ret.min = UICoord(Px(left), bounds.min.y);
        ret.max = UICoord(Px(left + thumb_width), bounds.max.y);
        return ret;
    }
}

} // namespace ui
} // namespace smlt
