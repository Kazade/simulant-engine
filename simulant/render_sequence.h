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

#include "renderers/renderer.h"
#include "types.h"
#include "viewport.h"
#include "partitioner.h"
#include "pipeline.h"

namespace smlt {

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class RenderSequence:
    public RefCounted<RenderSequence> {

public:
    RenderSequence(Window* window);
    virtual ~RenderSequence();

    PipelinePtr new_pipeline(
        const std::string& name,
        StageID stage,
        CameraID camera,
        const Viewport& viewport=Viewport(),
        TextureID target=TextureID(),
        int32_t priority=0
    );

    std::list<PipelinePtr>::iterator begin() {
        return ordered_pipelines_.begin();
    }

    std::list<PipelinePtr>::iterator end() {
        return ordered_pipelines_.end();
    }

    PipelinePtr find_pipeline(const std::string& name);
    bool destroy_pipeline(const std::string& name);
    void destroy_all_pipelines();
    void destroy_pipeline_immediately(const std::string& name);

    bool has_pipeline(const std::string& name);

    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer *renderer);

    void run();
    void clean_up();

    sig::signal<void (Pipeline&)>& signal_pipeline_started() { return signal_pipeline_started_; }
    sig::signal<void (Pipeline&)>& signal_pipeline_finished() { return signal_pipeline_finished_; }

    RenderOptions render_options;

    void destroy_object(PipelinePtr pipeline) {
        destroy_pipeline(pipeline->name());
    }

    void destroy_object_immediately(PipelinePtr pipeline) {
        // FIXME: This doesn't destroy immediately
        destroy_pipeline(pipeline->name());
    }

private:
    void sort_pipelines();
    void run_pipeline(PipelinePtr stage, int& actors_rendered);

    Window* window_ = nullptr;
    Renderer* renderer_ = nullptr;
    batcher::RenderQueue render_queue_;

    std::list<std::shared_ptr<Pipeline>> pool_;
    std::list<PipelinePtr> ordered_pipelines_;
    std::set<PipelinePtr> queued_for_destruction_;

    sig::signal<void (Pipeline&)> signal_pipeline_started_;
    sig::signal<void (Pipeline&)> signal_pipeline_finished_;

    friend class Pipeline;

    std::set<RenderTarget*> targets_rendered_this_frame_;

    sig::connection clean_up_connection_;
public:
    S_DEFINE_PROPERTY(window, &RenderSequence::window_);
};

}

#endif // PIPELINE_H
