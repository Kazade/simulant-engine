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

    LayerPtr new_pipeline(const std::string& name,
        StageNode* subtree,
        CameraPtr camera,
        const Viewport& viewport=Viewport(),
        TexturePtr target=nullptr,
        int32_t priority=0
    );

    LayerPtr render(StageNode* subtree, CameraPtr camera);

    std::list<LayerPtr>::iterator begin() {
        return ordered_pipelines_.begin();
    }

    std::list<LayerPtr>::iterator end() {
        return ordered_pipelines_.end();
    }

    LayerPtr find_pipeline(const std::string& name);
    bool destroy_pipeline(const std::string& name);
    void destroy_all_pipelines();
    void destroy_pipeline_immediately(const std::string& name);

    bool has_pipeline(const std::string& name);

    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer *renderer);

    void run();
    void clean_destroyed_pipelines();

    void destroy_object(LayerPtr pipeline) {
        destroy_pipeline(pipeline->name());
    }

    void destroy_object_immediately(LayerPtr pipeline) {
        // FIXME: This doesn't destroy immediately
        destroy_pipeline(pipeline->name());
    }

private:
    void sort_pipelines();
    void run_pipeline(LayerPtr stage, int& actors_rendered);

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

}

#endif // PIPELINE_H
