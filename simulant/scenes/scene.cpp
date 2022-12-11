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

#include "scene.h"
#include "../compositor.h"
#include "../stage.h"
#include "../window.h"
#include "../pipeline.h"
#include "../application.h"
#include "../platform.h"

namespace smlt {

SceneBase::SceneBase(Window *window):
    StageManager(),
    window_(window),
    input_(window->input.get()),
    app_(window->application),
    compositor_(window->compositor) {
}

SceneBase::~SceneBase() {

}

void SceneBase::_update_thunk(float dt) {
    if(!window->has_focus()) {
        // if paused, send deltatime as 0.0.
        // it's still accessible through window->time_keeper if the user needs it
        dt = 0.0;
    }

    StageManager::update(dt);
    update(dt);
}

void SceneBase::_fixed_update_thunk(float dt) {
    if(!window->has_focus()) return;

    StageManager::fixed_update(dt);
    fixed_update(dt);
}

std::size_t SceneBase::load_arg_count() const {
    return load_args.size();
}


void SceneBase::_call_load() {
    if(is_loaded_) {
        return;
    }

    auto memory_usage = smlt::get_app()->ram_usage_in_bytes();
    auto stage_node_capacity = smlt::get_app()->stage_node_pool_capacity();

    pre_load();
    load();

    auto used = smlt::get_app()->ram_usage_in_bytes();
    auto used_nodes = smlt::get_app()->stage_node_pool_capacity();
    if(smlt::get_app()->config->development.additional_memory_logging) {
        S_INFO("Loading scene {0} Memory usage {1} (before) vs {2} (after)", name(), memory_usage, used);
        if(used_nodes != stage_node_capacity) {
            S_INFO(
                "Stage node pool capacity change from {0} to {1}",
                stage_node_capacity, used_nodes
            );
        }
    }

    is_loaded_ = true;
}

void SceneBase::_call_unload() {
    if(!is_loaded_) {
        return;
    }

    _call_deactivate();

    auto memory_usage = smlt::get_app()->ram_usage_in_bytes();
    auto stage_node_capacity = smlt::get_app()->stage_node_pool_capacity();

    is_loaded_ = false;
    unload();
    post_unload();

    /* Make sure all stages have been destroyed */
    destroy_all_stages();       
    clean_destroyed_stages();

    smlt::get_app()->shared_assets->run_garbage_collection();

    auto n = name();
    auto used = smlt::get_app()->ram_usage_in_bytes();
    auto used_nodes = smlt::get_app()->stage_node_pool_capacity();
    if(smlt::get_app()->config->development.additional_memory_logging) {
        S_INFO("Unloading scene {0} Memory usage {1} (before) vs {2} (after)", n, memory_usage, used);
        if(used_nodes != stage_node_capacity) {
            S_INFO(
                "Stage node pool capacity change from {0} to {1}",
                stage_node_capacity, used_nodes
            );
        }
    }
}

void SceneBase::_call_activate() {
    if(is_active_) {
        return;
    }

    activate();
    is_active_ = true;
    signal_activated_();

    for(auto name: linked_pipelines_) {
        compositor->find_pipeline(name)->activate();
    }
}

void SceneBase::_call_deactivate() {
    if(!is_active_) {
        return;
    }

    for(auto name: linked_pipelines_) {
        compositor->find_pipeline(name)->deactivate();
    }
    linked_pipelines_.clear();

    deactivate();
    is_active_ = false;
    signal_deactivated_();
}

void SceneBase::link_pipeline(const std::string &name) {
    linked_pipelines_.insert(name);
}

void SceneBase::unlink_pipeline(const std::string &name) {
    linked_pipelines_.insert(name);
}

void SceneBase::link_pipeline(PipelinePtr pipeline) {
    link_pipeline(pipeline->name());
}

void SceneBase::unlink_pipeline(PipelinePtr pipeline) {
    unlink_pipeline(pipeline->name());
}



}

