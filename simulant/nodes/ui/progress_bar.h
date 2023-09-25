#pragma once

#include "widget.h"

namespace smlt {
namespace ui {

enum ProgressBarMode {
    PROGRESS_BAR_MODE_PULSE,
    PROGRESS_BAR_MODE_FRACTION
};

struct ProgressBarParams : public WidgetParams {
    float min = 0.0f;
    float max = 100.0f;
    float value = 0.0f;

    Px width;
    Px height;

    ProgressBarParams(
        float min=0.0f, float max=100.0f, float value=0.0f,
        Px width = Px(250),
        Px height = Rem(1.5f),
        const UIConfig& theme=UIConfig(),
        WidgetStylePtr shared_style=WidgetStylePtr()
    ):
        WidgetParams(theme, shared_style),
        min(min), max(max), value(value), width(width), height(height) {}
};

class ProgressBar:
    public Widget {

public:
    struct Meta {
        typedef ui::ProgressBarParams params_type;
        const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_PROGRESS_BAR;
    };

    using Widget::init; // Pull in init to satisfy Managed<Button>
    using Widget::clean_up;

    ProgressBar(Scene *owner);
    virtual ~ProgressBar();

    void pulse();
    void set_pulse_step(Px value);
    void set_pulse_fraction(float value);

    void set_fraction(float fraction);
    void set_value(float value);
    void set_range(float min, float max);

    float value() const;
    float min() const;
    float max() const;

    ProgressBarMode current_mode() const { return mode_; }

    void on_update(float dt) override;
private:
    bool on_create(void *params) override;

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

    WidgetBounds calculate_foreground_size(const UIDim& content_dimensions) const override;
};

}
}
