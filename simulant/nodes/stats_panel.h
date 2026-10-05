/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <list>

#include "../generic/managed.h"
#include "../panels/panel.h"
#include "../types.h"
#include "simulant/utils/params.h"

namespace smlt {

class Window;

class StatsPanel: public Panel, public RefCounted<StatsPanel> {

public:
    S_DEFINE_STAGE_NODE_META("stats_panel");

    StatsPanel(Scene* owner);

    bool on_init() override;
    void on_clean_up() override;

private:
    void update_stats();
    void rebuild_ram_graph(float total_mb);

    /* A translucent backdrop with two left-aligned multi-line labels on it
     * (the key and value columns) and the RAM graph below them. All text
     * uses the engine's embedded font, so the panel never needs a file. */
    ui::Label* backdrop_ = nullptr;
    ui::Label* keys_ = nullptr;
    ui::Label* values_ = nullptr;

    MaterialPtr graph_material_;
    MeshPtr ram_graph_mesh_;
    ActorPtr ram_graph_;
    std::list<float> ram_history_;

    /* Graph rectangle, in panel space */
    float graph_x_ = 0.0f;
    float graph_y_ = 0.0f;
    float graph_w_ = 0.0f;
    float graph_h_ = 0.0f;

    /* Totals over the current update interval */
    float elapsed_ = 0.0f;
    uint32_t frames_ = 0;
    float min_dt_ = 0.0f;
    float max_dt_ = 0.0f;
    uint64_t polygons_ = 0;
    uint64_t vertices_ = 0;
    uint64_t draws_ = 0;

    sig::connection frame_started_;
    bool first_update_ = true;

    bool on_create(Params params) override {
        _S_UNUSED(params);
        return true;
    }
};

} // namespace smlt
