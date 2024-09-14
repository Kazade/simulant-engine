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

#include <unordered_map>

#include "generic/algorithm.h"
#include "compositor.h"
#include "nodes/actor.h"
#include "nodes/camera.h"
#include "nodes/light.h"
#include "meshes/mesh.h"
#include "window.h"
#include "partitioner.h"
#include "loader.h"
#include "application.h"
#include "scenes/scene.h"
#include "renderers/batching/render_queue.h"
#include "tools/profiler.h"

namespace smlt {

Compositor::Compositor(Window *window):
    window_(window),
    renderer_(window->renderer.get()) {

}

Compositor::~Compositor() {
    clean_up_connection_.disconnect();
    destroy_all_layers();
}

LayerPtr Compositor::find_layer(const std::string &name) {
    for(auto& pipeline: ordered_pipelines_) {
        if(pipeline->name() == name) {
            return pipeline;
        }
    }

    return nullptr;
}

void Compositor::clean_destroyed_layers() {
    for(auto pip: queued_for_destruction_) {
        pip->deactivate();

#ifndef NDEBUG
        auto c = ordered_pipelines_.size();
#endif
        ordered_pipelines_.remove(pip);
#ifndef NDEBUG
        assert(ordered_pipelines_.size() < c);
#endif

        auto id = pip->id_;
        pool_.remove_if([id](const Layer::ptr& pip) -> bool {
            return pip->id_ == id;
        });
    }
    queued_for_destruction_.clear();
}


class TraceWriter : public batcher::RenderQueueVisitor {
public:
    TraceWriter(Renderer* renderer, std::ostream* out) :
        RenderQueueVisitor(renderer), out_(out) {}

    void start_traversal(const batcher::RenderQueue&, uint64_t, StageNode*) {}

    void change_render_group(const batcher::RenderGroup*, const batcher::RenderGroup* next) {
        group_ = next;
    }

    void change_material_pass(const MaterialPass*, const MaterialPass*) {}
    void apply_lights(const LightPtr*, const uint8_t) {}
    void do_visit(const Renderable* r, const MaterialPass*,
                  batcher::Iteration) override {
        auto line = _F("{0}, {1}, {2}, {3}, {4}\n").format(
            r,
            (group_) ? group_->sort_key.is_blended : false,
            (group_) ? group_->sort_key.distance_to_camera : -1.0f,
            r->render_priority,
            (group_) ? group_->sort_key.precedence : -1
        );

        out_->write(line.c_str(), line.size());
    }

    virtual void end_traversal(const batcher::RenderQueue&, StageNode*) {}

private:
    std::ostream* out_ = nullptr;
    const batcher::RenderGroup* group_ = nullptr;
};

void Compositor::dump_render_trace(Renderer* renderer, std::ostream* out) {
    auto visitor = std::make_shared<TraceWriter>(renderer, out);

    std::string headings = "RENDERABLE, BLENDED?, DISTANCE, PRIORITY, Z-ORDER\n";
    out->write(headings.c_str(), headings.size());

    sig::Connection conn = signal_layer_render_finished().connect([=](Layer&) {
        std::string row = ", , , ,\n";
        out->write(row.c_str(), row.size());

        render_queue_.traverse(visitor.get(), 0);
    });

    smlt::get_app()->signal_frame_finished().connect_once([=]() {
        auto conn2 = conn;
        out->flush();
        conn2.disconnect();
    });
}

void Compositor::destroy_all_layers() {
    auto pipelines = ordered_pipelines_;
    for(auto pip: pipelines) {
        assert(pip);
        destroy_object(pip);
    }
}

bool Compositor::has_layer(const std::string& name) {
    return bool(find_layer(name));
}

void Compositor::sort_layers() {
    auto do_sort = [&]() {
        ordered_pipelines_.sort(
            [](LayerPtr lhs, LayerPtr rhs) { return lhs->priority() < rhs->priority(); }
        );
    };

    do_sort();
}

LayerPtr Compositor::create_layer(
    StageNode* subtree, CameraPtr camera,
    const Viewport& viewport, TexturePtr target, int32_t priority) {

    auto pipeline = Layer::create(
        this, subtree, camera
    );

    /* New pipelines should always start deactivated to avoid the attached stage
     * as being updated automatically in the main thread when the pipeline
     * is constructed */
    pipeline->deactivate();
    pipeline->set_viewport(viewport);
    pipeline->set_target(target);
    pipeline->set_priority(priority);

    pool_.push_back(pipeline);

    ordered_pipelines_.push_back(pipeline.get());
    sort_layers();

    return pipeline.get();
}

void Compositor::set_renderer(Renderer* renderer) {
    renderer_ = renderer;
}

void Compositor::run() {
    S_VERBOSE("Running compositor");

    _S_PROFILE_SECTION("clean");
    clean_destroyed_layers();  /* Clean up any destroyed pipelines before rendering */

    targets_rendered_this_frame_.clear();

    /* Perform any pre-rendering tasks */
    renderer_->pre_render();

    int actors_rendered = 0;
    {        
        _S_PROFILE_SUBSECTION("pipelines");
        for(auto& pipeline: ordered_pipelines_) {
            run_layer(pipeline, actors_rendered);
        }
    }

    _S_PROFILE_SECTION("stats-update");
    get_app()->stats->set_subactors_rendered(actors_rendered);
}


uint64_t generate_frame_id() {
    static uint64_t frame_id = 0;
    return ++frame_id;
}

static bool build_renderables(
    std::vector<Light*>& lights_visible, batcher::RenderQueue* render_queue_,
    const smlt::CameraPtr& camera, const smlt::LayerPtr& pipeline_stage,
    StageNode* node
) {
    assert(node);

    if(!node->is_visible()) {
        return true;
    }

    std::partial_sort(
        lights_visible.begin(),
        lights_visible.begin() + std::min(MAX_LIGHTS_PER_RENDERABLE, (uint32_t) lights_visible.size()),
        lights_visible.end(),
        [=](LightPtr lhs, LightPtr rhs) {
            /* FIXME: Sorting by the center point is problematic. A renderable is made up
                 * of many polygons, by choosing the light closest to the center you may find that
                 * that polygons far away from the center aren't affected by lights when they should be.
                 * This needs more thought, probably. */
            if(lhs->light_type() == LIGHT_TYPE_DIRECTIONAL &&
               rhs->light_type() != LIGHT_TYPE_DIRECTIONAL) {
                return true;
            } else if(rhs->light_type() == LIGHT_TYPE_DIRECTIONAL &&
                      lhs->light_type() != LIGHT_TYPE_DIRECTIONAL) {
                return false;
            }

            float lhs_dist = (node->center() - lhs->transform->position()).length_squared();
            float rhs_dist = (node->center() - rhs->transform->position()).length_squared();
            return lhs_dist < rhs_dist;
        }
    );

    float distance_to_camera = camera->transform->position().distance_to(node->transformed_aabb());

    /* Find the ideal detail level at this distance from the camera */
    auto level = pipeline_stage->detail_level_at_distance(distance_to_camera);

    /* Push any renderables for this node */
    auto viewport = pipeline_stage->viewport.get();
    auto initial = render_queue_->renderable_count();
    node->generate_renderables(render_queue_, camera, viewport, level);

    // FIXME: Change get_renderables to return the number inserted
    auto count = render_queue_->renderable_count() - initial;

    for(auto i = initial; i < initial + count; ++i) {
        auto renderable = render_queue_->renderable(i);

        assert(
            renderable->arrangement == MESH_ARRANGEMENT_LINES ||
            renderable->arrangement == MESH_ARRANGEMENT_LINE_STRIP ||
            renderable->arrangement == MESH_ARRANGEMENT_QUADS ||
            renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES ||
            renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN ||
            renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP
        );

        assert(renderable->material);
        assert(renderable->vertex_data);

        renderable->light_count = std::min((std::size_t) MAX_LIGHTS_PER_RENDERABLE, lights_visible.size());
        for(auto i = 0u; i < renderable->light_count; ++i) {
            renderable->lights_affecting_this_frame[i] = lights_visible[i];
        }
    }

    return !(node->generates_renderables_for_descendents());
}

void Compositor::run_layer(LayerPtr pipeline_stage, int &actors_rendered) {
    /*
     * This is where rendering actually happens.
     *
     * FIXME: This needs some serious thought regarding thread-safety. There is
     * no locking here and another thread could be adding/removing objects,
     * updating the partitioner, or changing materials and/or textures on
     * renderables. We need to make sure that we render a consistent snapshot of
     * the world which means figuring out some kind of locking around the render
     * queue building and traversal, or some deep-copying (of
     * materials/textures/renderables) to make sure that nothing changes during
     * traversal
     */
    _S_PROFILE_SECTION("check");
    uint64_t frame_id = generate_frame_id();

    if(!pipeline_stage->is_active()) {
        return;
    }

    if(!pipeline_stage->is_complete()) {
        S_DEBUG("Stage or camera has been destroyed, disabling pipeline");
        pipeline_stage->deactivate();
        return;
    }

    auto stage_node = pipeline_stage->stage_node();
    auto camera = pipeline_stage->camera();

    RenderTarget& target = *window_; // FIXME: Should be window or texture

    /*
     *  Render targets can specify whether their buffer should be cleared at the
     * start of each frame. We do this the first time we hit a render target
     * when processing the pipelines. We keep track of the targets that have
     * been rendered each frame and this list is cleared at the start of run().
     */
    _S_PROFILE_SECTION("clear");
    if(targets_rendered_this_frame_.find(&target) ==
       targets_rendered_this_frame_.end()) {
        if(target.clear_every_frame_flags()) {
            Viewport view(smlt::VIEWPORT_TYPE_FULL,
                          target.clear_every_frame_color());
            renderer_->apply_viewport(target, view);
            renderer_->clear(target, view.color(),
                             target.clear_every_frame_flags());
        }

        targets_rendered_this_frame_.insert(&target);
    }

    auto& viewport = pipeline_stage->viewport;

    uint32_t clear = pipeline_stage->clear_flags();
    renderer_->apply_viewport(target, viewport);

    if(clear) {
        renderer_->clear(target, viewport->color(), clear);
    }

    signal_layer_render_started_(*pipeline_stage);

    // Trigger a signal to indicate the stage is about to be rendered
    stage_node->scene->signal_layer_render_started()(camera, viewport,
                                                     stage_node);

    static std::vector<Light*> lights_visible;

    /* Empty out, but leave capacity to prevent constant allocations */
    lights_visible.resize(0);

    _S_PROFILE_SECTION("gather-lights");
    /* Gather lights */
    StageNodeVisitorBFS light_finder(stage_node, [&](StageNode* node) {
        if(node->node_type() == STAGE_NODE_TYPE_DIRECTIONAL_LIGHT ||
           node->node_type() == STAGE_NODE_TYPE_POINT_LIGHT) {
            Light* light = (Light*)node;
            if(light->light_type() == LIGHT_TYPE_DIRECTIONAL) {
                lights_visible.push_back(light);
            } else {
                if(camera->frustum().intersects_sphere(
                       light->transform->position(), light->diameter())) {
                    lights_visible.push_back(light);
                }
            }
        }
    });

    // Traverse the tree in BFS order
    while(light_finder.call_next()) {}

    _S_PROFILE_SECTION("reset");
    // Reset it, ready for this pipeline
    render_queue_.reset(stage_node, window->renderer.get(), camera);

    _S_PROFILE_SUBSECTION("build-renderables");
    StageNodeVisitorBFS node_finder(
        stage_node, std::bind(build_renderables, lights_visible, &render_queue_,
                              camera, pipeline_stage, std::placeholders::_1));

    while(node_finder.call_next()) {}

    actors_rendered += render_queue_.renderable_count();

    using namespace std::placeholders;

    _S_PROFILE_SECTION("traverse");
    auto visitor = renderer_->get_render_queue_visitor(camera);

    // Render the visible objects
    render_queue_.traverse(visitor.get(), frame_id);

    _S_PROFILE_SECTION("post-render");
    // Trigger a signal to indicate the stage has been rendered
    stage_node->scene->signal_layer_render_finished()(camera, viewport,
                                                      stage_node);

    signal_layer_render_finished_(*pipeline_stage);
    render_queue_.clear();
}

SceneCompositor::SceneCompositor(Scene* scene, Compositor* global_compositor):
    compositor_(global_compositor),
    scene_(scene) {

    activate_connection_ = scene_->signal_activated().connect([=]() {
        for(auto& layer: layers_) {
            if(layer->activation_mode() == LAYER_ACTIVATION_MODE_AUTOMATIC) {
                layer->activate();
            }
        }
    });

    deactivate_connection_ = scene_->signal_deactivated().connect([=]() {
        for(auto& layer: layers_) {
            if(layer->activation_mode() == LAYER_ACTIVATION_MODE_AUTOMATIC) {
                layer->deactivate();
            }
        }
    });
}

SceneCompositor::~SceneCompositor() {
    activate_connection_.disconnect();
    deactivate_connection_.disconnect();

    destroy_all_layers();
}

}
