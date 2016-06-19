#include "stats_panel.h"
#include "../window_base.h"

namespace kglt {

StatsPanel::StatsPanel(WindowBase *window):
    window_(window) {

}

void StatsPanel::initialize() {
    if(initialized_) return;

    ui_stage_id_ = window_->new_ui_stage();
    ui_camera_ = window_->new_camera_with_orthographic_projection(0, 640, 480, 0);
    pipeline_id_ = window_->render(ui_stage_id_, ui_camera_).with_priority(kglt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);

    initialized_ = true;
}

void StatsPanel::do_activate() {
    initialize();

    window_->enable_pipeline(pipeline_id_);
    L_DEBUG("Activating stats panel");
}

void StatsPanel::do_deactivate() {
    window_->disable_pipeline(pipeline_id_);
    L_DEBUG("Deactivating stats panel");
}

}
