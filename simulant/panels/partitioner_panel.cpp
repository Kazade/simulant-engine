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


#include "partitioner_panel.h"
#include "../application.h"
#include "../window.h"
#include "../stage.h"
#include "../nodes/actor.h"
#include "../partitioner.h"

namespace smlt {

PartitionerPanel::PartitionerPanel(Window* window):
    window_(window) {

}

void PartitionerPanel::do_activate() {
    for(auto& pair: debug_actors_) {
        pair.second->set_visible(true);
    }
}

void PartitionerPanel::do_deactivate() {
    for(auto& pair: debug_actors_) {
        pair.second->set_visible(false);
    }
}

bool PartitionerPanel::init() {
    auto scene = get_app()->scenes->active_scene();

    /* for stage in stages: stage->new_actor(stage->partitioner->debug_mesh_id()) */
    for(auto stage: scene->each_stage()) {
        // Don't add another mesh if it already exists
        if(debug_actors_.count(stage->id())) {
            continue;
        }

        debug_actors_[stage->id()] = stage->new_actor_with_mesh(
            stage->partitioner->debug_mesh_id()
        );

        debug_actors_[stage->id()]->set_cullable(false);
    }

    stage_added_ = scene->signal_stage_added().connect([=](StageID stage_id) {
        auto stage = scene->stage(stage_id);

        debug_actors_[stage->id()] = stage->new_actor_with_mesh(
            stage->partitioner->debug_mesh_id()
        );

        debug_actors_[stage->id()]->set_cullable(false);
    });

    stage_removed_ = scene->signal_stage_removed().connect([=](StageID stage_id) {
        debug_actors_.erase(stage_id);
    });

    return true;
}

void PartitionerPanel::clean_up() {
    stage_added_.disconnect();
    stage_removed_.disconnect();
}


}
