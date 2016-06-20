#include "stats_panel.h"
#include "../window_base.h"
#include "../ui_stage.h"

namespace kglt {

StatsPanel::StatsPanel(WindowBase *window):
    window_(window) {

}

void StatsPanel::initialize() {
    if(initialized_) return;

    ui_stage_id_ = window_->new_ui_stage();
    ui_camera_ = window_->new_camera_with_orthographic_projection(0, 640, 480, 0);
    pipeline_id_ = window_->render(ui_stage_id_, ui_camera_).with_priority(kglt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);

    auto overlay = window_->ui_stage(ui_stage_id_);

    auto body = overlay->append("<body/>");
    body.css("color", "#4BD3FFDD");

    auto frame_time = body.append("<div></div>");
    frame_time.css("top", "1.5em");
    frame_time.css("position", "absolute");
    frame_time.css("margin-left", "1em");
    frame_time.html("Frame time: <span id='frame-time'></span>");
    overlay->find("#frame-time").text("0");

    auto fps = body.append("<div></div>");
    fps.css("top", "3em");
    fps.css("margin-left", "1em");
    fps.css("position", "absolute");
    fps.html("FPS: <span id='fps'></span>");
    overlay->find("#fps").text("0");

    window_->signal_frame_started().connect(std::bind(&StatsPanel::update, this));

    initialized_ = true;
}

void StatsPanel::update() {
    auto overlay = window_->ui_stage(ui_stage_id_);
    overlay->find("#fps").text(
        _u("{0}").format(window_->stats->frames_per_second())
    );
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
