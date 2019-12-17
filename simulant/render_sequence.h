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

template<typename T, typename IDType, typename ...Subtypes>
class ManualManager;

typedef ManualManager<Pipeline, PipelineID> PipelineManager;

class RenderSequence:
    public RefCounted<RenderSequence> {

public:
    RenderSequence(Window* window);
    virtual ~RenderSequence();

    PipelinePtr new_pipeline(
        StageID stage,
        CameraID camera,
        const Viewport& viewport=Viewport(),
        TextureID target=TextureID(),
        int32_t priority=0
    );

    PipelinePtr pipeline(PipelineID pipeline);
    void destroy_pipeline(PipelineID pipeline);
    void destroy_all_pipelines();
    bool has_pipeline(PipelineID pipeline);

    PipelinePtr find_pipeline_with_name(const std::string& name);

    void activate_pipelines(const std::vector<PipelineID>& pipelines);
    std::vector<PipelineID> active_pipelines() const;
    void deactivate_all_pipelines();

    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer *renderer);

    void run();

    sig::signal<void (Pipeline&)>& signal_pipeline_started() { return signal_pipeline_started_; }
    sig::signal<void (Pipeline&)>& signal_pipeline_finished() { return signal_pipeline_finished_; }

    RenderOptions render_options;

    Property<RenderSequence, Window> window = { this, &RenderSequence::window_ };

    void destroy_object(PipelinePtr pipeline) {
        destroy_pipeline(pipeline->id());
    }

    void destroy_object_immediately(PipelinePtr pipeline) {
        // FIXME: This doesn't destroy immediately
        destroy_pipeline(pipeline->id());
    }

private:    
    void sort_pipelines(bool acquire_lock=false);
    void run_pipeline(PipelinePtr stage, int& actors_rendered);

    Window* window_ = nullptr;
    Renderer* renderer_ = nullptr;

    batcher::RenderQueue render_queue_;
    std::shared_ptr<RenderableStore> renderable_store_;

    std::mutex pipeline_lock_;
    std::list<PipelinePtr> ordered_pipelines_;

    sig::signal<void (Pipeline&)> signal_pipeline_started_;
    sig::signal<void (Pipeline&)> signal_pipeline_finished_;

    friend class Pipeline;

    std::set<RenderTarget*> targets_rendered_this_frame_;

    sig::connection clean_up_connection_;

    std::unique_ptr<PipelineManager> pipeline_manager_;
};

}

#endif // PIPELINE_H
