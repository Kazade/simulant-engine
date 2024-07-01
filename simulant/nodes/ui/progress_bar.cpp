#include "progress_bar.h"
#include "../../stage.h"
#include "../../window.h"
#include "simulant/utils/construction_args.h"
#include "ui_manager.h"

namespace smlt {
namespace ui {

ProgressBar::ProgressBar(Scene* owner) :
    Widget(owner, STAGE_NODE_TYPE_WIDGET_PROGRESS_BAR) {}

ProgressBar::~ProgressBar() {}

void ProgressBar::set_pulse_fraction(float value) {
    pulse_fraction_ = value;
}

void ProgressBar::refresh_pulse(float dt) {
    Px bar_width = outer_width() - (border_width() * 2);

    pulse_width_ = bar_width.value * pulse_fraction_;
    float pulse_range = (bar_width.value - pulse_width_);
    float pulse_max = pulse_range / 2.0f;
    float pulse_min = -pulse_range / 2.0f;

    if(pulse_right_) {
        pulse_position_ += pulse_step_ * dt;
    } else {
        pulse_position_ -= pulse_step_ * dt;
    }

    if(pulse_position_ <= pulse_min) {
        pulse_position_ = pulse_min;
        pulse_right_ = true;
    } else if(pulse_position_ >= pulse_max) {
        pulse_position_ = pulse_max;
        pulse_right_ = false;
    }

    rebuild();
}

void ProgressBar::refresh_fraction() {
    auto new_fraction = value() / (max() - min());

    // Don't update for tiny changes
    if(std::abs(new_fraction - fraction_) > 0.01f) {
        fraction_ = new_fraction;
        rebuild();
    }
}

void ProgressBar::refresh_bar(float dt) {
    if(!needs_refresh_) {
        return;
    }

    needs_refresh_ = false;

    if(current_mode() == PROGRESS_BAR_MODE_PULSE) {
        refresh_pulse(dt);
    } else {
        refresh_fraction();
    }
}

Widget::WidgetBounds ProgressBar::calculate_foreground_size(
    const UIDim& content_dimensions) const {
    WidgetBounds result = Widget::calculate_foreground_size(content_dimensions);

    if(mode_ == PROGRESS_BAR_MODE_PULSE) {
        result.min.x = Px(pulse_position_ - (pulse_width_ / 2));
        result.max.x = Px(pulse_position_ + (pulse_width_ / 2));
    } else {
        result.max.x =
            result.min.x +
            int(float((result.max.x - result.min.x).value) * fraction_);
    }
    return result;
}

void ProgressBar::pulse() {
    mode_ = PROGRESS_BAR_MODE_PULSE;
    needs_refresh_ = true;
}

void ProgressBar::set_pulse_step(Px value) {
    pulse_step_ = std::abs(value.value);
}

void ProgressBar::set_range(float min, float max) {
    assert(min < max);

    min_ = min;
    max_ = max;
    needs_refresh_ = true;
}

void ProgressBar::set_value(float value) {
    if(value != this->value()) {
        needs_refresh_ = true;
        value_ = value;
    }
}

void ProgressBar::set_fraction(float fraction) {
    auto value = min() + (max() * fraction);
    set_value(value);
}

float ProgressBar::value() const {
    return value_;
}

float ProgressBar::min() const {
    return min_;
}

float ProgressBar::max() const {
    return max_;
}

void ProgressBar::on_update(float dt) {
    refresh_bar(dt);
}

bool ProgressBar::on_create(Params params) {
    if(!Widget::on_create(params)) {
        return true;
    }

    auto sstyle = params.arg<WidgetStyle>("shared_style");
    auto theme = params.arg<UIConfig>("theme").value_or(UIConfig());

    if(!sstyle) {
        set_background_color(theme.progress_bar_background_color_);
        set_foreground_color(theme.progress_bar_foreground_color_);
        set_border_color(theme.progress_bar_border_color_);
        set_border_width(theme.progress_bar_border_width_);
        set_text_color(theme.progress_bar_text_color_);
    }

    auto min = params.arg<float>("min").value_or(0.0f);
    auto max = params.arg<float>("max").value_or(100.0f);
    auto value = params.arg<float>("value").value_or(0.0f);
    set_range(min, max);
    set_value(value);

    auto w = params.arg<int>("width").value_or(-1);
    auto h = params.arg<int>("height").value_or(-1);

    set_resize_mode(RESIZE_MODE_FIXED);
    resize(w, h);
    return true;
}
} // namespace ui
} // namespace smlt
