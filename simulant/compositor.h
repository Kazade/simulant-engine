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

#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <memory>
#include <list>

#include "generic/managed.h"
#include "generic/property.h"
#include "signals/signal.h"
#include "renderers/renderer.h"
#include "types.h"
#include "viewport.h"
#include "partitioner.h"
#include "layer.h"

namespace smlt {

typedef sig::signal<void (Layer&)> LayerRenderStarted;
typedef sig::signal<void (Layer&)> LayerRenderFinished;

class Compositor:
    public RefCounted<Compositor> {

    DEFINE_SIGNAL(LayerRenderStarted, signal_layer_render_started);
    DEFINE_SIGNAL(LayerRenderFinished, signal_layer_render_finished);

public:
    Compositor(Window* window);
    virtual ~Compositor();

    LayerPtr create_layer(
        StageNode* subtree,
        CameraPtr camera,
        const Viewport& viewport=Viewport(),
        TexturePtr target=nullptr,
        int32_t priority=0
    );

    std::list<LayerPtr>::iterator begin() {
        return ordered_pipelines_.begin();
    }

    std::list<LayerPtr>::iterator end() {
        return ordered_pipelines_.end();
    }

    LayerPtr find_layer(const std::string& name);
    void destroy_all_layers();

    bool has_layer(const std::string& name);

    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer *renderer);

    void run();
    void clean_destroyed_layers();

    void destroy_object(LayerPtr pip) {
        if(queued_for_destruction_.count(pip)) {
            return;
        }

        queued_for_destruction_.insert(pip);

        /* When a user requests destruction, we deactivate immediately
         * as that's the path of least surprise. The pipeline won't be used
         * anyway on the next render, this just makes sure that the stage for example
         * doesn't think it's part of an active pipeline until then */
        pip->deactivate();
        pip->destroy();
    }

    void destroy_object_immediately(LayerPtr pipeline) {
        // FIXME: This doesn't destroy immediately
        destroy_object(pipeline);
    }

    /* Writes to file the render queue for the current frame in CSV
     * format. The `out` parameter must stay valid for the lifetime
     * of the frame */
    void dump_render_trace(std::ostream* out);

private:
    void sort_layers();
    void run_layer(LayerPtr stage, int& actors_rendered);

    Window* window_ = nullptr;
    Renderer* renderer_ = nullptr;
    batcher::RenderQueue render_queue_;

    std::list<std::shared_ptr<Layer>> pool_;
    std::list<LayerPtr> ordered_pipelines_;
    std::set<LayerPtr> queued_for_destruction_;

    friend class Layer;

    std::set<RenderTarget*> targets_rendered_this_frame_;

    sig::connection clean_up_connection_;
public:
    S_DEFINE_PROPERTY(window, &Compositor::window_);
};


/**
 * @brief The SceneCompositor class
 *
 * The Compositor class is global, across the whole application. The
 * SceneCompositor is a scene-local equivalent which ensures that layers do
 * not outlive the scene.
 */

class SceneCompositor {
public:
    SceneCompositor(Scene* scene, Compositor* global_compositor);

    ~SceneCompositor();

    LayerPtr create_layer(StageNode* subtree, Camera* camera, int32_t priority=0) {
        auto ret = compositor_->create_layer(subtree, camera, Viewport(), nullptr, priority);
        layers_.push_back(ret);
        return ret;
    }

    LayerPtr find_layer(const std::string& name) {
        for(auto& layer: layers_) {
            if(layer->name() == name) {
                return layer;
            }
        }

        return LayerPtr();
    }

    void destroy_all_layers() {
        for(auto& layer: layers_) {
            layer->destroy();
        }
    }

    std::list<LayerPtr>::iterator begin() {
        return layers_.begin();
    }

    std::list<LayerPtr>::iterator end() {
        return layers_.end();
    }

private:
    Compositor* compositor_ = nullptr;
    Scene* scene_ = nullptr;

    std::list<LayerPtr> layers_;

    sig::Connection activate_connection_;
    sig::Connection deactivate_connection_;

public:
    S_DEFINE_PROPERTY(global_compositor, &SceneCompositor::compositor_);
};

}

#endif // PIPELINE_H
