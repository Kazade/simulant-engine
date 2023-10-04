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
#include "../services/service.h"
#include "../nodes/actor.h"
#include "../nodes/geom.h"
#include "../nodes/camera.h"
#include "../nodes/light.h"
#include "../nodes/mesh_instancer.h"
#include "../nodes/particle_system.h"
#include "../nodes/rigid_body.h"
#include "../nodes/static_body.h"
#include "../nodes/kinematic_body.h"
#include "../nodes/sprite.h"
#include "../nodes/smooth_follow.h"
#include "../nodes/fly_controller.h"
#include "../nodes/skies/skybox.h"
#include "../nodes/ui/image.h"
#include "../nodes/ui/label.h"
#include "../nodes/ui/button.h"
#include "../nodes/ui/frame.h"
#include "../nodes/ui/keyboard.h"
#include "../nodes/ui/text_entry.h"
#include "../nodes/ui/progress_bar.h"

#include "../nodes/cylindrical_billboard.h"
#include "../nodes/spherical_billboard.h"
#include "../partitioners/frustum_partitioner.h"

namespace smlt {

Scene::Scene(Window *window):
    StageNode(this, STAGE_NODE_TYPE_SCENE),
    StageNodeManager(this),
    window_(window),
    input_(window->input.get()),
    app_(window->application),
    compositor_(window->compositor),
    assets_(window->application->shared_assets.get()) {

    register_builtin_nodes();
}

Scene::~Scene() {
    for(auto& child: each_descendent()) {
        child.destroy();
    }

    clean_up_destroyed_objects();
}

void Scene::register_builtin_nodes() {
    register_stage_node<Stage>();
    register_stage_node<Actor>();
    register_stage_node<Geom>();
    register_stage_node<Camera>();
    register_stage_node<DirectionalLight>();
    register_stage_node<PointLight>();
    register_stage_node<MeshInstancer>();
    register_stage_node<FrustumPartitioner>();
    register_stage_node<CylindricalBillboard>();
    register_stage_node<SphericalBillboard>();
    register_stage_node<ParticleSystem>();
    register_stage_node<Sprite>();
    register_stage_node<Skybox>();
    register_stage_node<SmoothFollow>();
    register_stage_node<FlyController>();

    register_stage_node<StaticBody>();
    register_stage_node<RigidBody>();
    register_stage_node<KinematicBody>();

    register_stage_node<ui::Label>();
    register_stage_node<ui::Image>();
    register_stage_node<ui::ProgressBar>();
    register_stage_node<ui::Frame>();
    register_stage_node<ui::Button>();
    register_stage_node<ui::TextEntry>();
    register_stage_node<ui::Keyboard>();
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

    signal_stage_node_destroyed_(node, node->node_type());
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

void Scene::on_fixed_update(float step) {
    /* Update services, before moving onto the scene tree */
    for(auto& service: services_) {
        service.second->fixed_update(step);
    }

    StageNode::on_fixed_update(step);
}



}

