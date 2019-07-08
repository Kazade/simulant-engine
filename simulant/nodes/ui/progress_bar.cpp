
#include "progress_bar.h"
#include "../../stage.h"
#include "../../window.h"

namespace smlt {
namespace ui {


ProgressBar::ProgressBar(WidgetID id, UIManager* owner, UIConfig* config):
    Widget(id, owner, config) {

    set_background_colour(config->progress_bar_background_colour_);
    set_foreground_colour(config->progress_bar_foreground_colour_);
    set_border_colour(config->progress_bar_border_colour_);
    set_border_width(config->progress_bar_border_width_);

    idle_connection_ = this->stage->window->idle->add([this]() -> bool {
        refresh_bar();
        return true;
    });
}

ProgressBar::~ProgressBar() {
    this->stage->window->idle->remove(idle_connection_);
}

void ProgressBar::set_pulse_fraction(float value) {
    pulse_fraction_ = value;
}

void ProgressBar::refresh_pulse() {
    pulse_width_ = this->content_width() * pulse_fraction_;
    float pulse_range = (this->content_width() - pulse_width_);
    float pulse_max = pulse_range / 2.0f;
    float pulse_min = -pulse_range / 2.0f;

    float dt = stage->window->time_keeper->delta_time();

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

    resize_or_generate_foreground(mesh(), pulse_width_, content_height(), pulse_position_, 0);
}

void ProgressBar::refresh_fraction() {

}

void ProgressBar::refresh_bar() {
    if(!needs_refresh_) {
        return;
    }

    needs_refresh_ = false;

    if(current_mode() == PROGRESS_BAR_MODE_PULSE) {
        refresh_pulse();
    } else {
        refresh_fraction();
    }
}

void ProgressBar::pulse() {
    mode_ = PROGRESS_BAR_MODE_PULSE;
    needs_refresh_ = true;
}

void ProgressBar::set_pulse_step(float value) {
    pulse_step_ = std::fabs(value);
}

void ProgressBar::set_fraction(float fraction) {
    resize_or_generate_foreground(mesh(), content_width() * fraction, content_height(), 0, 0);
}

}
}
