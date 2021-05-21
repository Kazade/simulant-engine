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
    using Widget::clean_up;

    ProgressBar(UIManager* owner, UIConfig* config);
    virtual ~ProgressBar();

    void pulse();
    void set_pulse_step(float value);
    void set_pulse_fraction(float value);

    void set_fraction(float fraction);
    void set_value(float value);
    void set_range(float min, float max);

    float value() const;
    float min() const;
    float max() const;

    ProgressBarMode current_mode() const { return mode_; }

    void update(float dt) override;
private:
    ProgressBarMode mode_ = PROGRESS_BAR_MODE_FRACTION;

    float value_ = 0.0f;
    float min_ = 0.0f;
    float max_ = 100.0f;

    float pulse_position_ = 0.0f;
    float pulse_step_ = 300.0f;
    float pulse_fraction_ = 0.33f;
    bool pulse_right_ = true;
    float pulse_width_ = 1.0f;
    float fraction_ = 0.0f;

    bool needs_refresh_ = true;

    void refresh_pulse(float dt);
    void refresh_fraction();
    void refresh_bar(float dt);

    WidgetBounds calculate_foreground_size(float content_width, float content_height) const override;
};

}
}
