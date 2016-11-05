#include "stats_panel.h"
#include "../window_base.h"
#include "../overlay.h"
#include "../stage.h"

namespace kglt {

StatsPanel::StatsPanel(WindowBase *window):
    window_(window) {

}

void StatsPanel::initialize() {
    if(initialized_) return;

    overlay_id_ = window_->new_overlay();
    ui_camera_ = window_->new_camera_with_orthographic_projection(0, 640, 480, 0);
    pipeline_id_ = window_->render(overlay_id_, ui_camera_).with_priority(kglt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);

    auto overlay = window_->overlay(overlay_id_);

    overlay->add_css("color", "#4BD3FFDD");

    overlay->append_row().append_label("Performance");

    auto fps = overlay->append_row();
    fps.append_label("FPS: ");
    fps.append_label("0").set_id("fps");

    auto ram_usage = overlay->append_row();
    ram_usage.append_label("RAM: ");
    ram_usage.append_label("0").set_id("ram");

    overlay->append_row().set_id("stages");

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
        auto overlay = window_->overlay(overlay_id_);
        overlay->find("#fps").set_text(
            _u("{0}").format(window_->stats->frames_per_second())
        );

        auto mem_usage = get_memory_usage_in_megabytes();
        overlay->find("#ram").set_text(
            _u("{0} MB").format(mem_usage)
        );

        last_update = 0.0f;
        first_update = false;

        auto stages = overlay->find("#stages");
        stages.remove_children();

        this->window_->each_stage([&](uint32_t i, Stage* stage) {
            auto stage_row = stages.append_row();
            stage_row.append_row().append_label(
                (stage->name().empty()) ? "Stage " + std::to_string(i) : stage->name().encode()
            );
            stage_row.append_row().append_label(
                "   Actors: " + std::to_string(stage->actor_count())
            );

            stage_row.append_row().append_label(
                "   Particle Systems: " + std::to_string(stage->particle_system_count())
            );
        });
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
