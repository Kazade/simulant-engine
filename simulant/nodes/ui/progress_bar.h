#pragma once

#include "simulant/nodes/stage_node.h"
#include "simulant/utils/params.h"
#include "widget.h"

namespace smlt {
namespace ui {

enum ProgressBarMode {
    PROGRESS_BAR_MODE_PULSE,
    PROGRESS_BAR_MODE_FRACTION
};

class ProgressBar: public Widget {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_PROGRESS_BAR,
                             "progress_bar");

    S_DEFINE_STAGE_NODE_PARAM(ProgressBar, "min", float, 0.0f,
                              "The minimum value of the progress bar");
    S_DEFINE_STAGE_NODE_PARAM(ProgressBar, "max", float, 100.0f,
                              "The maximum value of the progress bar");
    S_DEFINE_STAGE_NODE_PARAM(ProgressBar, "value", float, 0.0f,
                              "The current value of the progress bar");

    S_DEFINE_CORE_WIDGET_PROPERTIES(ProgressBar);

    using Widget::clean_up;
    using Widget::init; // Pull in init to satisfy Managed<Button>

    ProgressBar(Scene* owner);
    virtual ~ProgressBar();

    void pulse();
    void set_pulse_step(Px value);
    void set_pulse_fraction(float value);

    void set_fraction(NormalizedFloat fraction);
    void set_value(float value);
    void set_range(float min, float max);

    float value() const;
    float min() const;
    float max() const;

    ProgressBarMode current_mode() const {
        return mode_;
    }

    void on_update(float dt) override;

private:
    bool on_create(Params params) override;

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

    WidgetBounds calculate_foreground_size(
        const UIDim& content_dimensions) const override;
};

} // namespace ui
} // namespace smlt
