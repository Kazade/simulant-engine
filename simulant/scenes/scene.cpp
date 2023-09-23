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
#include "../asset_manager.h"

namespace smlt {

Scene::Scene(Window *window):
    StageNode(nullptr, STAGE_NODE_TYPE_SCENE),
    StageNodeManager(this),
    window_(window),
    input_(window->input.get()),
    app_(window->application),
    compositor_(window->compositor) {
}

Scene::~Scene() {

}

void Scene::clean_up_destroyed_objects() {
    for(auto it = queued_for_clean_up_.begin(); it != queued_for_clean_up_.end();) {
        if(clean_up_node((*it))) {
            it = queued_for_clean_up_.erase(it);
        } else {
            S_ERROR("Unable to release node that has not been destroyed");
            ++it;
        }
    }
}

void Scene::queue_clean_up(StageNode* node) {
    if(node == this) {
        // You can't destroy the scene, only the scene manager can do that
        return;
    }

    queued_for_clean_up_.push_back(node);
}

std::size_t Scene::load_arg_count() const {
    return load_args.size();
}


static void print_asset_stats() {
    const auto& sa = smlt::get_app()->shared_assets;
    S_INFO("Shared assets: ");
    S_INFO("- Meshes: {0}", sa->mesh_count());
    S_INFO("- Textures: {0}", sa->texture_count());
    S_INFO("- Sounds: {0}", sa->sound_count());
    S_INFO("- Fonts: {0}", sa->font_count());
    S_INFO("- Particle Scripts: {0}", sa->particle_script_count());
    S_INFO("---");

    for(std::size_t i = 0; i < sa->child_manager_count(); ++i) {
        auto c = sa->child_manager(i);
        S_INFO("   {0}", c);
        S_INFO("  - Meshes: {0}", c->mesh_count());
        S_INFO("  - Textures: {0}", c->texture_count());
        S_INFO("  - Sounds: {0}", c->sound_count());
        S_INFO("  - Fonts: {0}", c->font_count());
        S_INFO("  - Particle Scripts: {0}", c->particle_script_count());
        S_INFO("---");
    }
}


void Scene::_call_load() {
    if(is_loaded_) {
        return;
    }

    auto memory_usage = smlt::get_app()->ram_usage_in_bytes();

    pre_load();
    load();

    auto used = smlt::get_app()->ram_usage_in_bytes();
    if(smlt::get_app()->config->development.additional_memory_logging) {
        S_INFO("Loading scene {0} Memory usage {1} (before) vs {2} (after)", name(), memory_usage, used);
        print_asset_stats();
    }

    is_loaded_ = true;
}

void Scene::_call_unload() {
    if(!is_loaded_) {
        return;
    }

    _call_deactivate();

    auto memory_usage = smlt::get_app()->ram_usage_in_bytes();

    is_loaded_ = false;
    unload();
    post_unload();

    smlt::get_app()->shared_assets->run_garbage_collection();

    auto n = name();
    auto used = smlt::get_app()->ram_usage_in_bytes();

    if(smlt::get_app()->config->development.additional_memory_logging) {
        S_INFO("Unloading scene {0} Memory usage {1} (before) vs {2} (after)", n, memory_usage, used);
    }
}

void Scene::_call_activate() {
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

void Scene::_call_deactivate() {
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

void Scene::link_pipeline(const std::string &name) {
    linked_pipelines_.insert(name);
}

void Scene::unlink_pipeline(const std::string &name) {
    linked_pipelines_.insert(name);
}

void Scene::link_pipeline(PipelinePtr pipeline) {
    link_pipeline(pipeline->name());
}

void Scene::unlink_pipeline(PipelinePtr pipeline) {
    unlink_pipeline(pipeline->name());
}



}

