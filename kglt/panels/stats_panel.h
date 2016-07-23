#pragma once

#include "panel.h"
#include "../types.h"

namespace kglt {

class WindowBase;

class StatsPanel : public Panel {
public:
    StatsPanel(WindowBase* window);

private:
    WindowBase* window_ = nullptr;

    void do_activate() override;
    void do_deactivate() override;
    void initialize();
    bool initialized_ = false;

    UIStageID ui_stage_id_;
    CameraID ui_camera_;
    PipelineID pipeline_id_;

    void update();

    int32_t get_memory_usage_in_megabytes();
};

}
