//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
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
#include "../application.h"
#include "../compositor.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../nodes/ui/label.h"
#include "../nodes/ui/ui_manager.h"
#include "../platform.h"
#include "../stage.h"
#include "../time_keeper.h"
#include "../window.h"
#include "simulant/utils/construction_args.h"

#if defined(__WIN32__)
#include <psapi.h>
#include <windows.h>
#endif

namespace smlt {

StatsPanel::StatsPanel(Scene* owner) :
    Panel(owner, STAGE_NODE_TYPE_STATS_PANEL) {}

bool StatsPanel::on_init() {
    if(!Panel::on_init()) {
        return false;
    }

    auto hw = 32;
    auto label_width = ui::Px(scene->window->width() * 0.5f);

    const float diff = 32;
    float vheight = scene->window->height() - diff;

    auto heading1 = create_child<ui::Label>(
        Args({"text", "performance", "width", label_width}));
    heading1->transform->set_position_2d(Vec2(hw, vheight));
    vheight -= diff;

    fps_ =
        create_child<ui::Label>(Args({"text", "FPS: 0", "width", label_width}));
    fps_->transform->set_position_2d(Vec2(hw, vheight));
    vheight -= diff;

    frame_time_ = create_child<ui::Label>(Args(
        {"text", "Frame Time: 0ms", "width", label_width}
    ));
    frame_time_->transform->set_position_2d(Vec2(hw, vheight));
    vheight -= diff;

    ram_usage_ = create_child<ui::Label>(
        Args({"text", "RAM Used: 0", "width", label_width}));

    ram_usage_->transform->set_position_2d(Vec2(hw, vheight));
    vheight -= diff;

    vram_usage_ = create_child<ui::Label>(
        Args({"text", "VRAM Used: 0", "width", label_width}));
    vram_usage_->transform->set_position_2d(Vec2(hw, vheight));
    vheight -= diff;

    actors_rendered_ = create_child<ui::Label>(
        Args({"text", "Renderables visible: 0", "width", label_width}));
    actors_rendered_->transform->set_position_2d(Vec2(hw, vheight));
    vheight -= diff;

    polygons_rendered_ = create_child<ui::Label>(
        Args({"text", "Polygons Rendered: 0", "width", label_width}));
    polygons_rendered_->transform->set_position_2d(Vec2(hw, vheight));

    graph_material_ =
        scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
    graph_material_->set_blend_func(BLEND_ALPHA);
    graph_material_->set_depth_test_enabled(false);
    graph_material_->set_cull_mode(CULL_MODE_NONE);
    ram_graph_mesh_ =
        scene->assets->create_mesh(smlt::VertexSpecification::DEFAULT);
    ram_graph_ = create_child<Actor>(Args({"mesh", ram_graph_mesh_}));
    ram_graph_->set_cullable(false);

    low_mem_ = create_child<ui::Label>(Args({"text", "0M"}));
    high_mem_ = create_child<ui::Label>(Args({"text", "0M"}));

    frame_started_ = get_app()->signal_frame_started().connect(
        std::bind(&StatsPanel::update_stats, this));

    return true;
}

void StatsPanel::on_clean_up() {
    frame_started_.disconnect();

    Panel::on_clean_up();

    fps_ = nullptr;
    frame_time_ = nullptr;
    ram_usage_ = nullptr;
    actors_rendered_ = nullptr;
    polygons_rendered_ = nullptr;
}

static float bytes_to_megabytes(uint64_t bytes) {
    float m = 1.0f / 1024.0f;
    if(bytes == MEMORY_VALUE_UNAVAILABLE) {
        return 0;
    }

    return float(bytes) * m * m;
}

int32_t StatsPanel::get_memory_usage_in_megabytes() {
    return bytes_to_megabytes(get_app()->ram_usage_in_bytes());
}

#ifndef __DREAMCAST__
static unsigned int round(unsigned int value, unsigned int multiple) {
    return ((value - 1u) & ~(multiple - 1u)) + multiple;
}
#endif

#define RAM_SAMPLES 25

void StatsPanel::rebuild_ram_graph() {
    smlt::Color color = smlt::Color::BLUE;
    color.a = 0.35;

    float width = scene->window->width();
    float height = 64;

    ram_graph_mesh_->reset(
        ram_graph_mesh_->vertex_data->vertex_specification());

    if(free_ram_history_.size() < 2) {
        // Can't make a graph with a single sample point
        return;
    }

#ifdef __DREAMCAST__
    float graph_max = 16.0f;
#else
    float max_y =
        *(std::max_element(free_ram_history_.begin(), free_ram_history_.end()));
    float graph_max = round(max_y, 8.0f);
#endif

    if(almost_equal(graph_max, 0.0f)) {
        // Prevent divide by zero
        return;
    }

    auto submesh = ram_graph_mesh_->create_submesh("ram-usage", graph_material_,
                                                   INDEX_TYPE_16_BIT,
                                                   MESH_ARRANGEMENT_QUADS);
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

    auto last_sample_it = free_ram_history_.begin();
    auto this_sample_it = last_sample_it;
    this_sample_it++;

    for(; this_sample_it != free_ram_history_.end(); ++this_sample_it) {
        auto last_sample = *last_sample_it;
        auto sample = *this_sample_it;

        float y = (height / graph_max) * last_sample;
        vdata->position(x, y, 0);
        vdata->diffuse(color);
        vdata->move_next();
        idata->index(idx++);

        vdata->position(x, 0, 0);
        vdata->diffuse(color);
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
        vdata->position(x, 0, 0);
        vdata->diffuse(color);
        vdata->move_next();
        idata->index(idx++);

        vdata->position(x, y, 0);
        vdata->diffuse(color);
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

        last_sample_it = this_sample_it;
    }

    low_mem_->set_text(_F("{0}M").format(lowest_mem));
    low_mem_->transform->set_position_2d(Vec2(lowest_x, lowest_y + 10));

    high_mem_->set_text(_F("{0}M").format(highest_mem));
    high_mem_->transform->set_position_2d(Vec2(highest_x, highest_y + 10));

    vdata->done();
    idata->done();
}

void StatsPanel::update_stats() {
    last_update_ += get_app()->time_keeper->delta_time();

    if(first_update_ || last_update_ >= 1.0f) {
        auto mem_usage = get_memory_usage_in_megabytes();
        auto tot_mem = bytes_to_megabytes(get_platform()->total_ram_in_bytes());
        auto vram_usage =
            bytes_to_megabytes(get_platform()->available_vram_in_bytes());
        auto actors_rendered = get_app()->stats->subactors_rendered();

        free_ram_history_.push_back(mem_usage);
        if(free_ram_history_.size() > RAM_SAMPLES) {
            free_ram_history_.pop_front();
        }

        rebuild_ram_graph();

        fps_->set_text(
            _F("FPS: {0}").format(get_app()->stats->frames_per_second()));
        frame_time_->set_text(
            _F("Frame Time: {0}ms").format(get_app()->stats->frame_time()));
        ram_usage_->set_text(
            _F("RAM Usage: {0} / {1} MB").format(mem_usage, tot_mem));
        vram_usage_->set_text(_F("VRAM Free: {0} MB").format(vram_usage));
        actors_rendered_->set_text(
            _F("Renderables Visible: {0}").format(actors_rendered));
        polygons_rendered_->set_text(
            _F("Polygons Rendered: {0}")
                .format(get_app()->stats->polygons_rendered()));

        last_update_ = 0.0f;
        first_update_ = false;

        /* FIXME: Restore this...
         *
        auto stages = overlay->find("#stages");
        stages.remove_children();

        this->window_->each_stage([&](uint32_t i, Stage* stage) {
            auto stage_row = stages.append_row();
            stage_row.append_row().append_label(
                (stage->name().empty()) ? "Stage " + smlt::to_string(i) :
        stage->name().encode()
            );
            stage_row.append_row().append_label(
                "   Actors: " + smlt::to_string(stage->actor_count())
            );

            stage_row.append_row().append_label(
                "   Particle Systems: " +
        smlt::to_string(stage->particle_system_count())
            );
        }); */
    }
}

} // namespace smlt
