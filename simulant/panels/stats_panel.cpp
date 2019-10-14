//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "stats_panel.h"
#include "../window.h"
#include "../stage.h"
#include "../nodes/ui/ui_manager.h"
#include "../render_sequence.h"
#include "../nodes/ui/label.h"

#if defined(__WIN32__)
#include <windows.h>
#include <psapi.h>
#endif

namespace smlt {

StatsPanel::StatsPanel(Window *window):
    window_(window) {

}

bool StatsPanel::init() {
    stage_ = window_->new_stage();

    ui_camera_ = stage_->new_camera_with_orthographic_projection(0, window_->width(), 0, window_->height());
    pipeline_ = window_->render(stage_, ui_camera_).with_priority(smlt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);
    pipeline_->deactivate();

    auto overlay = stage_;

    auto hw = 32;
    float label_width = window_->width() * 0.5f;

    const float diff = 32;
    float vheight = window_->height() - diff;

    auto heading1 = overlay->ui->new_widget_as_label("Performance", label_width);
    heading1->move_to(hw, vheight);
    vheight -= diff;

    fps_ = overlay->ui->new_widget_as_label("FPS: 0", label_width);
    fps_->move_to(hw, vheight);
    vheight -= diff;

    frame_time_ = overlay->ui->new_widget_as_label("Frame Time: 0ms", label_width);
    frame_time_->move_to(hw, vheight);
    vheight -= diff;

    ram_usage_ = overlay->ui->new_widget_as_label("RAM: 0", label_width);
    ram_usage_->move_to(hw, vheight);
    vheight -= diff;

    actors_rendered_ = overlay->ui->new_widget_as_label("Renderables visible: 0", label_width);
    actors_rendered_->move_to(hw, vheight);
    vheight -= diff;

    polygons_rendered_ = overlay->ui->new_widget_as_label("Polygons Rendered: 0", label_width);
    polygons_rendered_->move_to(hw, vheight);
    vheight -= diff;

    graph_material_ = stage_->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);
    graph_material_->set_blend_func(BLEND_ALPHA);
    graph_material_->set_depth_test_enabled(false);
    ram_graph_mesh_ = stage_->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
    ram_graph_ = stage_->new_actor_with_mesh(ram_graph_mesh_, RENDERABLE_CULLING_MODE_NEVER);

    low_mem_ = overlay->ui->new_widget_as_label("0M");
    high_mem_ = overlay->ui->new_widget_as_label("0M");

    frame_started_ = window_->signal_frame_started().connect(std::bind(&StatsPanel::update, this));

    return true;
}

void StatsPanel::clean_up() {
    frame_started_.disconnect();

    if(pipeline_) {
        window_->destroy_pipeline(pipeline_);
    }

    if(stage_) {
        stage_->destroy();
        stage_ = nullptr;
    }

    fps_ = nullptr;
    frame_time_ = nullptr;
    ram_usage_ = nullptr;
    actors_rendered_ = nullptr;
    polygons_rendered_ = nullptr;
}

#ifdef _arch_dreamcast
#include <malloc.h>
#include <kos.h>

static unsigned long systemRam = 0x00000000;
static unsigned long elfOffset = 0x00000000;
static unsigned long stackSize = 0x00000000;

extern unsigned long end;
extern unsigned long start;

#define _end end
#define _start start

void set_system_ram() {
   systemRam = 0x8d000000 - 0x8c000000;
   elfOffset = 0x8c000000;

   stackSize = (long)&_end - (long)&_start + ((long)&_start - elfOffset);
}

unsigned long get_system_ram() {
   return systemRam;
}

unsigned long get_free_ram() {
    struct mallinfo mi = mallinfo();
    return systemRam - (mi.usmblks + stackSize);
}

#endif

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
#elif defined(_arch_dreamcast)
    return float(get_system_ram() - get_free_ram()) / 1024.0f / 1024.0f;
#elif defined(__WIN32__)
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return static_cast<float>(info.WorkingSetSize) / 1024.0f / 1024.0f;
#else
    return -1;
#endif
}

#define RAM_SAMPLES 25

static unsigned int round(unsigned int value, unsigned int multiple){
    return ((value-1u) & ~(multiple-1u)) + multiple;
}

void StatsPanel::rebuild_ram_graph() {
    smlt::Colour colour = smlt::Colour::SKY_BLUE;
    colour.a = 0.35;

    float width = window_->width();
    float height = window_->height() * 0.4f;

    ram_graph_mesh_->clear();

    if(free_ram_history_.size() < 2) {
        // Can't make a graph with a single sample point
        return;
    }

#ifdef _arch_dreamcast
    float graph_max = 16.0f;
#else
    float max_y = *(std::max_element(free_ram_history_.begin(), free_ram_history_.end()));
    float graph_max = round(max_y, 8.0f);
#endif

    auto submesh = ram_graph_mesh_->new_submesh_with_material("ram-usage", graph_material_, MESH_ARRANGEMENT_QUADS);
    auto& vdata = ram_graph_mesh_->vertex_data;
    auto& idata = submesh->index_data;

    float x = 0;
    float xstep = width / (RAM_SAMPLES - 1);

    float lowest_mem = std::numeric_limits<float>::max();
    float lowest_x = 0;
    float lowest_y = 0;

    float highest_mem = std::numeric_limits<float>::lowest();
    float highest_x = 0;
    float highest_y = 0;

    auto idx = vdata->count();
    for(auto i = 1u; i < free_ram_history_.size(); ++i) {
        auto sample = free_ram_history_[i];
        auto last_sample = free_ram_history_[i - 1];

        float y = (height / graph_max) * last_sample;
        vdata->position(x, y, -1);
        vdata->diffuse(colour);
        vdata->move_next();
        idata->index(idx++);

        vdata->position(x, 0, -1);
        vdata->diffuse(colour);
        vdata->move_next();
        idata->index(idx++);

        if(last_sample < lowest_mem) {
            lowest_x = x;
            lowest_y = y;
            lowest_mem = last_sample;
        }

        if(last_sample > highest_mem) {
            highest_x = x;
            highest_y = y;
            highest_mem = last_sample;
        }

        x += xstep;

        y = (height / graph_max) * sample;
        vdata->position(x, 0, -1);
        vdata->diffuse(colour);
        vdata->move_next();
        idata->index(idx++);

        vdata->position(x, y, -1);
        vdata->diffuse(colour);
        vdata->move_next();
        idata->index(idx++);

        if(sample < lowest_mem) {
            lowest_x = x;
            lowest_y = y;
            lowest_mem = sample;
        }

        if(sample > highest_mem) {
            highest_x = x;
            highest_y = y;
            highest_mem = sample;
        }
    }

    low_mem_->set_text(_u("{0}M").format(lowest_mem));
    low_mem_->move_to(lowest_x, lowest_y + 10);

    high_mem_->set_text(_u("{0}M").format(highest_mem));
    high_mem_->move_to(highest_x, highest_y + 10);

    vdata->done();
    idata->done();
}

void StatsPanel::update() {
    last_update_ += window_->time_keeper->delta_time();

    if(first_update_ || last_update_ >= 1.0) {
        auto mem_usage = get_memory_usage_in_megabytes();
        auto actors_rendered = window_->stats->geometry_visible();

        free_ram_history_.push_back(mem_usage);
        if(free_ram_history_.size() > RAM_SAMPLES) {
            free_ram_history_.pop_front();
        }

        rebuild_ram_graph();

        fps_->set_text(_u("FPS: {0}").format(window_->stats->frames_per_second()));
        frame_time_->set_text(_F("Frame Time: {0}ms").format(window_->stats->frame_time()));
        ram_usage_->set_text(_u("RAM: {0} MB").format(mem_usage));
        actors_rendered_->set_text(_u("Renderables Visible: {0}").format(actors_rendered));
        polygons_rendered_->set_text(_u("Polygons Rendered: {0}").format(window_->stats->polygons_rendered()));

        last_update_ = 0.0f;
        first_update_ = false;

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
    pipeline_->activate();
    L_DEBUG("Activating stats panel");
}

void StatsPanel::do_deactivate() {
    pipeline_->deactivate();
    L_DEBUG("Deactivating stats panel");
}

}
