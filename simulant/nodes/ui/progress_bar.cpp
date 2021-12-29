#include "ui_manager.h"
#include "progress_bar.h"
#include "../../stage.h"
#include "../../window.h"

namespace smlt {
namespace ui {


ProgressBar::ProgressBar(UIManager* owner, UIConfig* config):
    Widget(owner, config) {

    set_background_colour(config->progress_bar_background_colour_);
    set_foreground_colour(config->progress_bar_foreground_colour_);
    set_border_colour(config->progress_bar_border_colour_);
    set_border_width(config->progress_bar_border_width_);
    set_text_colour(config->progress_bar_text_colour_);

    resize(config->progress_bar_width_, config->progress_bar_height_);
}

ProgressBar::~ProgressBar() {

}

void ProgressBar::set_pulse_fraction(float value) {
    pulse_fraction_ = value;
}

void ProgressBar::refresh_pulse(float dt) {
    pulse_width_ = this->content_width().value * pulse_fraction_;
    float pulse_range = (this->content_width().value - pulse_width_);
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

Widget::WidgetBounds ProgressBar::calculate_foreground_size() const {
    WidgetBounds result = Widget::calculate_foreground_size();
    if(mode_ == PROGRESS_BAR_MODE_PULSE) {
        result.min.x = pulse_position_ - (pulse_width_ / 2);
        result.max.x = pulse_position_ + (pulse_width_ / 2);
    } else {
        result.max.x = result.min.x + ((result.max.x - result.min.x) * fraction_);
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

void ProgressBar::update(float dt) {
    refresh_bar(dt);
}

}
}
