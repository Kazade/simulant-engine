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

    auto title = body.append("<h1></h1>");
    title.css("width", "640px");
    title.css("position", "absolute");
    title.css("top", "1.5em");
    title.css("text-align", "center");
    title.text("Performance");

    auto fps = body.append("<div></div>");
    fps.css("top", "3em");
    fps.css("margin-left", "1em");
    fps.css("position", "absolute");
    fps.html("FPS: <span id='fps'></span>");
    overlay->find("#fps").text("0");

    auto ram_usage = body.append("<div></div>");
    ram_usage.css("top", "4.5em");
    ram_usage.css("margin-left", "1em");
    ram_usage.css("position", "absolute");
    ram_usage.html("RAM: <span id='ram'></span>");
    overlay->find("#ram").text("0");

    window_->signal_frame_started().connect(std::bind(&StatsPanel::update, this));

    initialized_ = true;
}

int32_t StatsPanel::get_memory_usage_in_megabytes() {
#ifdef __linux__
    std::ifstream file("/proc/self/status");
    std::string line;
    while(std::getline(file, line)) {

        if(unicode(line).starts_with("VmRSS:")) {
            auto parts = unicode(line).split(" ", -1, false);
            if(parts.size() == 3) {
                return float(parts[1].to_int()) / 1024.0f;
            }
        }
    }
    return -1;
#else
    return -1;
#endif
}

void StatsPanel::update() {
    static float last_update = 0.0f;
    static bool first_update = true;

    last_update += window_->delta_time();

    if(first_update || last_update >= 1.0) {
        auto overlay = window_->ui_stage(ui_stage_id_);
        overlay->find("#fps").text(
            _u("{0}").format(window_->stats->frames_per_second())
        );

        auto mem_usage = get_memory_usage_in_megabytes();
        overlay->find("#ram").text(
            _u("{0} MB").format(mem_usage)
        );

        last_update = 0.0f;
        first_update = false;
    }
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
