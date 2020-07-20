/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
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

#include "panel.h"
#include "../types.h"
#include "../generic/managed.h"

namespace smlt {

class Core;

class StatsPanel:
    public Panel,
    public RefCounted<StatsPanel> {

public:
    StatsPanel(Core* core);

    bool init() override;
    void clean_up() override;
private:
    Core* core_ = nullptr;

    void do_activate() override;
    void do_deactivate() override;

    StagePtr stage_;
    CameraPtr ui_camera_;
    PipelinePtr pipeline_;

    void update();

    int32_t get_memory_usage_in_megabytes();

    ui::WidgetPtr fps_;
    ui::WidgetPtr frame_time_;
    ui::WidgetPtr ram_usage_;
    ui::WidgetPtr actors_rendered_;
    ui::WidgetPtr polygons_rendered_;

    MaterialPtr graph_material_;
    MeshPtr ram_graph_mesh_;
    ActorPtr ram_graph_;
    std::list<float> free_ram_history_;

    void rebuild_ram_graph();

    ui::WidgetPtr low_mem_;
    ui::WidgetPtr high_mem_;

    sig::connection frame_started_;
    bool first_update_ = true;
    float last_update_ = 0.0f;
};

}
