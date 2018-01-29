//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "stats_panel.h"
#include "../window.h"
#include "../stage.h"
#include "../nodes/ui/ui_manager.h"
#include "../render_sequence.h"
#include "../nodes/ui/label.h"

namespace smlt {

StatsPanel::StatsPanel(Window *window):
    window_(window) {

}

void StatsPanel::initialize() {
    if(initialized_) return;

    stage_ = window_->new_stage();

    ui_camera_ = stage_->new_camera_with_orthographic_projection(0, 640, 0, 480);
    pipeline_ = window_->render(stage_, ui_camera_).with_priority(smlt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);
    pipeline_->deactivate();

    auto overlay = stage_;

    float vheight = 460;
    const float diff = 32;

    auto heading1 = overlay->ui->new_widget_as_label("Performance");
    heading1->move_to(320, vheight);
    vheight -= diff;

    fps_ = overlay->ui->new_widget_as_label("FPS: 0");
    fps_->move_to(320, vheight);
    vheight -= diff;

    ram_usage_ = overlay->ui->new_widget_as_label("RAM: 0");
    ram_usage_->move_to(320, vheight);
    vheight -= diff;

    actors_rendered_ = overlay->ui->new_widget_as_label("Renderables visible: 0");
    actors_rendered_->move_to(320, vheight);
    vheight -= diff;

    polygons_rendered_ = overlay->ui->new_widget_as_label("Polygons Rendered: 0");
    polygons_rendered_->move_to(320, vheight);
    vheight -= diff;

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

    last_update += window_->time_keeper->delta_time();

    if(first_update || last_update >= 1.0) {
        auto mem_usage = get_memory_usage_in_megabytes();
        auto actors_rendered = window_->stats->geometry_visible();

        fps_->set_text(_u("FPS: {0}").format(window_->stats->frames_per_second()));
        ram_usage_->set_text(_u("RAM: {0} MB").format(mem_usage));
        actors_rendered_->set_text(_u("Renderables Visible: {0}").format(actors_rendered));
        polygons_rendered_->set_text(_u("Polygons Rendered: {0}").format(window_->stats->polygons_rendered()));

        last_update = 0.0f;
        first_update = false;

        /* FIXME: Restore this...
         *
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
        }); */
    }
}

void StatsPanel::do_activate() {
    initialize();

    pipeline_->activate();
    L_DEBUG("Activating stats panel");
}

void StatsPanel::do_deactivate() {
    pipeline_->deactivate();
    L_DEBUG("Deactivating stats panel");
}

}
