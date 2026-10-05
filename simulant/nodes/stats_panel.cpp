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

#include <algorithm>
#include <cstdio>
#include <iterator>

#include "stats_panel.h"
#include "../application.h"
#include "../asset_manager.h"
#include "../compositor.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../nodes/ui/label.h"
#include "../nodes/ui/ui_manager.h"
#include "../font.h"
#include "../platform.h"
#include "../renderers/renderer.h"
#include "../stats_recorder.h"
#include "../stage.h"
#include "../time_keeper.h"
#include "../window.h"
#include "simulant/utils/params.h"

#if defined(__WIN32__)
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#endif

namespace smlt {

StatsPanel::StatsPanel(Scene* owner) :
    Panel(owner, Meta::node_type) {}

/* The embedded font is a pixel font drawn on a 16px grid: it only renders
 * cleanly at that size */
static const int FONT_SIZE = 16;

static const float GRAPH_HEIGHT_LINES = 2.0f;
static const int COLUMN_GAP = 12;
static const int MARGIN = 8;
static const int PADDING = 6;
static const int GRAPH_GAP = 4;

static const char* KEY_TEXT = "STATS\nFPS\nFRAME\nDRAWS\nTRIS\nVERTS\nRAM\nVRAM";
static const int KEY_LINES = 8;

#define RAM_SAMPLES 30

static float bytes_to_megabytes(uint64_t bytes) {
    float m = 1.0f / 1024.0f;
    if(bytes == MEMORY_VALUE_UNAVAILABLE) {
        return 0;
    }

    return float(bytes) * m * m;
}

/* The width of the widest line of `text` in `font`, in pixels */
static float text_width(FontPtr font, const std::string& text) {
    float widest = 0.0f, line = 0.0f;
    for(std::size_t i = 0; i < text.size(); ++i) {
        if(text[i] == '\n') {
            widest = std::max(widest, line);
            line = 0.0f;
            continue;
        }
        const char32_t next = (i + 1 < text.size()) ? char32_t(text[i + 1]) : 0;
        line += font->character_advance(char32_t(text[i]), next);
    }
    return std::max(widest, line);
}

/* 1234 -> "1234", 12345 -> "12.3K", 1234567 -> "1.23M" */
static std::string format_count(double value) {
    char buf[32];
    if(value >= 1000000.0) {
        snprintf(buf, sizeof(buf), "%.2fM", value / 1000000.0);
    } else if(value >= 10000.0) {
        snprintf(buf, sizeof(buf), "%.1fK", value / 1000.0);
    } else {
        snprintf(buf, sizeof(buf), "%d", int(value + 0.5));
    }
    return buf;
}

bool StatsPanel::on_init() {
    if(!Panel::on_init()) {
        return false;
    }

    const float width = scene->window->width();
    const float height = scene->window->height();

    auto font = get_app()->embedded_font(FONT_SIZE);

    /* The labels are created with the default font (which is the embedded
     * one, preloaded) and then switched to the embedded font at our size, so
     * nothing is ever looked up on disk */
    ui::UIConfig theme;
    theme.label_resize_mode_ = ui::RESIZE_MODE_FIT_CONTENT;
    theme.label_background_color_ = Color::none();

    auto make_label = [&](const Color& text_color) -> ui::Label* {
        auto label = create_child<ui::Label>(Params()
            .set("text", std::string(""))
            .set("theme", theme));
        if(font) {
            label->set_font(font);
        }
        label->set_text_color(text_color);
        label->set_text_alignment(ui::TEXT_ALIGNMENT_LEFT);
        label->set_padding(ui::Px(0));
        label->set_anchor_point(0.0f, 1.0f);
        return label;
    };

    backdrop_ = make_label(Color::none());
    keys_ = make_label(Color(0.55f, 0.6f, 0.65f, 1.0f));
    values_ = make_label(Color(0.92f, 0.95f, 1.0f, 1.0f));
    keys_->set_text(KEY_TEXT);

    /* Size the columns to the text: the keys, and a stand-in for the widest
     * values (the real platform/renderer names and RAM size vary most) */
    const float total_mb = bytes_to_megabytes(get_platform()->total_ram_in_bytes());
    char ram[32];
    snprintf(ram, sizeof(ram), "999.9 / %.0f MB", double(total_mb));
    const std::string widest_values =
        get_platform()->name() + " / " + scene->window->renderer->name() +
        "\n999  (99.99 ms)\n99.9 - 99.9 ms\n9999\n999.9K  (999.9K/s)\n" + ram +
        "\n99.99 MB free";

    const float key_width = font ? text_width(font, KEY_TEXT) : keys_->content_width().value;
    const float value_width = font ? text_width(font, widest_values) : 0.0f;

    const float line_height = keys_->line_height().value;
    const float panel_width = std::min(PADDING + key_width + COLUMN_GAP + value_width + PADDING,
                                       width - 2 * MARGIN);
    const float top = height - MARGIN;
    const float text_top = top - PADDING;

    graph_x_ = MARGIN + PADDING;
    graph_w_ = panel_width - 2 * PADDING;
    graph_h_ = line_height * GRAPH_HEIGHT_LINES;
    graph_y_ = text_top - KEY_LINES * line_height - GRAPH_GAP - graph_h_;

    const float panel_height = (top - graph_y_) + PADDING;

    backdrop_->set_resize_mode(ui::RESIZE_MODE_FIXED);
    backdrop_->resize(ui::Px(int(panel_width)), ui::Px(int(panel_height)));
    backdrop_->set_background_color(Color(0.03f, 0.04f, 0.05f, 0.75f));
    backdrop_->transform->set_position_2d(Vec2(MARGIN, top));

    keys_->transform->set_position_2d(Vec2(MARGIN + PADDING, text_top));
    values_->transform->set_position_2d(
        Vec2(MARGIN + PADDING + key_width + COLUMN_GAP, text_top));

    /* Translucent widgets at equal distance sort by precedence: the
     * backdrop draws first */
    keys_->set_precedence(1);
    values_->set_precedence(1);

    graph_material_ =
        scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
    graph_material_->set_blend_func(BLEND_ALPHA);
    graph_material_->set_depth_test_enabled(false);
    graph_material_->set_cull_mode(CULL_MODE_NONE);
    ram_graph_mesh_ =
        scene->assets->create_mesh(smlt::VertexSpecification::DEFAULT);
    ram_graph_ = create_child<Actor>(ram_graph_mesh_);
    ram_graph_->set_cullable(false);
    ram_graph_->set_precedence(1);

    frame_started_ = get_app()->signal_frame_started().connect(
        std::bind(&StatsPanel::update_stats, this));

    return true;
}

void StatsPanel::on_clean_up() {
    frame_started_.disconnect();

    Panel::on_clean_up();

    backdrop_ = nullptr;
    keys_ = nullptr;
    values_ = nullptr;
}

void StatsPanel::rebuild_ram_graph(float total_mb) {
    ram_graph_mesh_->reset(
        ram_graph_mesh_->vertex_data->vertex_specification());

    /* Scale to the machine's RAM where that's small enough for the usage to
     * register (consoles), otherwise to the peak sample */
    const float peak = ram_history_.empty() ? 0.0f :
        *std::max_element(ram_history_.begin(), ram_history_.end());
    float graph_max = (total_mb > 0.0f && total_mb <= 64.0f) ? total_mb : peak * 1.25f;
    if(graph_max <= 0.0f) {
        graph_max = 1.0f;
    }

    auto submesh = ram_graph_mesh_->create_submesh("ram-usage", graph_material_,
                                                   INDEX_TYPE_16_BIT,
                                                   MESH_ARRANGEMENT_QUADS);
    auto& vdata = ram_graph_mesh_->vertex_data;
    auto& idata = submesh->index_data;

    const Color fill(0.25f, 0.6f, 1.0f, 0.45f);
    const Color track(1.0f, 1.0f, 1.0f, 0.06f);

    auto quad = [&](float x0, float y0, float x1, float y1, const Color& color) {
        auto i = vdata->count();
        vdata->position(x0, y1, 0); vdata->color(color); vdata->move_next();
        vdata->position(x0, y0, 0); vdata->color(color); vdata->move_next();
        vdata->position(x1, y0, 0); vdata->color(color); vdata->move_next();
        vdata->position(x1, y1, 0); vdata->color(color); vdata->move_next();
        for(int k = 0; k < 4; ++k) idata->index(i + k);
    };

    /* The graph's full extent, faintly, so it reads as a graph even before
     * there's much history */
    quad(graph_x_, graph_y_, graph_x_ + graph_w_, graph_y_ + graph_h_, track);

    if(ram_history_.size() < 2) {
        vdata->done();
        idata->done();
        return;
    }

    const float xstep = graph_w_ / (RAM_SAMPLES - 1);
    const float yscale = graph_h_ / graph_max;

    /* Right-aligned, so the newest sample is always at the right edge */
    float x = graph_x_ + graph_w_ - xstep * (ram_history_.size() - 1);
    auto idx = vdata->count();

    auto prev = ram_history_.begin();
    for(auto it = std::next(prev); it != ram_history_.end(); prev = it, ++it) {
        const float y0 = graph_y_ + std::min(*prev * yscale, graph_h_);
        const float y1 = graph_y_ + std::min(*it * yscale, graph_h_);

        vdata->position(x, y0, 0);
        vdata->color(fill);
        vdata->move_next();
        idata->index(idx++);

        vdata->position(x, graph_y_, 0);
        vdata->color(fill);
        vdata->move_next();
        idata->index(idx++);

        x += xstep;

        vdata->position(x, graph_y_, 0);
        vdata->color(fill);
        vdata->move_next();
        idata->index(idx++);

        vdata->position(x, y1, 0);
        vdata->color(fill);
        vdata->move_next();
        idata->index(idx++);
    }

    vdata->done();
    idata->done();
}

void StatsPanel::update_stats() {
    /* Called at the start of each frame, so the counters hold the previous
     * frame's totals and delta_time() is that frame's duration */
    auto app = get_app();
    const float dt = app->time_keeper->delta_time();

    if(frames_ == 0 || dt < min_dt_) min_dt_ = dt;
    if(frames_ == 0 || dt > max_dt_) max_dt_ = dt;
    elapsed_ += dt;
    frames_++;
    polygons_ += app->stats->polygons_rendered();
    vertices_ += app->stats->vertices_rendered();
    draws_ += app->stats->subactors_rendered();

    if(!first_update_ && elapsed_ < 1.0f) {
        return;
    }

    const float seconds = std::max(elapsed_, 1e-6f);
    const double per_frame = 1.0 / frames_;

    const float ram_mb = bytes_to_megabytes(app->ram_usage_in_bytes());
    const float total_mb = bytes_to_megabytes(get_platform()->total_ram_in_bytes());
    const uint64_t vram_free = get_platform()->available_vram_in_bytes();

    ram_history_.push_back(ram_mb);
    if(ram_history_.size() > RAM_SAMPLES) {
        ram_history_.pop_front();
    }
    rebuild_ram_graph(total_mb);

    char line[64];
    std::string text;

    text += get_platform()->name() + " / " + scene->window->renderer->name() + "\n";

    snprintf(line, sizeof(line), "%.0f  (%.2f ms)\n", double(frames_ / seconds),
             double(1000.0f * seconds / frames_));
    text += line;

    snprintf(line, sizeof(line), "%.1f - %.1f ms\n", double(min_dt_ * 1000.0f),
             double(max_dt_ * 1000.0f));
    text += line;

    text += format_count(draws_ * per_frame) + "\n";
    text += format_count(polygons_ * per_frame) + "  (" +
            format_count(polygons_ / seconds) + "/s)\n";
    text += format_count(vertices_ * per_frame) + "  (" +
            format_count(vertices_ / seconds) + "/s)\n";

    if(total_mb > 0.0f) {
        snprintf(line, sizeof(line), "%.1f / %.0f MB\n", double(ram_mb), double(total_mb));
    } else {
        snprintf(line, sizeof(line), "%.1f MB\n", double(ram_mb));
    }
    text += line;

    if(vram_free == MEMORY_VALUE_UNAVAILABLE) {
        text += "n/a";
    } else {
        snprintf(line, sizeof(line), "%.2f MB free",
                 double(bytes_to_megabytes(vram_free)));
        text += line;
    }

    values_->set_text(text);

    elapsed_ = 0.0f;
    frames_ = 0;
    polygons_ = 0;
    vertices_ = 0;
    draws_ = 0;
    first_update_ = false;
}

} // namespace smlt
