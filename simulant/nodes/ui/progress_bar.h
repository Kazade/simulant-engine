#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

enum ProgressBarMode {
    PROGRESS_BAR_MODE_PULSE,
    PROGRESS_BAR_MODE_FRACTION
};

class ProgressBar:
    public Widget {

public:
    using Widget::init; // Pull in init to satisfy Managed<Button>
    using Widget::cleanup;

    ProgressBar(WidgetID id, UIManager* owner, UIConfig* config);
    ~ProgressBar();

    void pulse();
    void set_pulse_step(float value);
    void set_pulse_fraction(float value);
    void set_fraction(float fraction);

    ProgressBarMode current_mode() const { return mode_; }

private:
    ProgressBarMode mode_ = PROGRESS_BAR_MODE_FRACTION;

    float pulse_position_ = 0.0f;
    float pulse_step_ = 150.0f;
    float pulse_fraction_ = 0.33f;
    bool pulse_right_ = true;
    float pulse_width_ = 1.0f;

    bool needs_refresh_ = true;

    void refresh_pulse();
    void refresh_fraction();
    void refresh_bar();

    IdleConnectionID idle_connection_;
};

}
}
