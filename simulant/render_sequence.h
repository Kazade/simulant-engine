/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <memory>
#include <list>

#include "generic/managed.h"
#include "generic/manager.h"
#include "generic/property.h"

#include "renderers/renderer.h"
#include "types.h"
#include "viewport.h"
#include "partitioner.h"

namespace smlt {

class RenderSequence;

class Pipeline:
    public Managed<Pipeline>,
    public generic::Identifiable<PipelineID>{

public:
    Pipeline(
        PipelineID id,
        RenderSequence* render_sequence
    );

    ~Pipeline();

    CameraID camera_id() { return camera_; }
    StageID stage_id() { return stage_; }
    TextureID target_id() { return target_; }
    uint32_t clear_flags() const { return clear_mask_; }

    int32_t priority() const { return priority_; }
    void set_priority(int32_t priority);

    void deactivate();
    void activate();
    bool is_active() const { return is_active_; }

    void set_stage(StageID s) { stage_ = s; }
    void set_camera(CameraID c) { camera_ = c; }
    void set_viewport(const Viewport& v) { viewport_ = v; }
    void set_target(TextureID t) { target_ = t; }
    void set_clear_flags(uint32_t viewport_clear_flags) {
        clear_mask_ = viewport_clear_flags;
    }

    DetailLevel detail_level_at_distance(float dist) const;

    Property<Pipeline, Viewport> viewport = { this, &Pipeline::viewport_ };
private:
    RenderSequence* sequence_;
    int32_t priority_;
    StageID stage_;
    TextureID target_;
    CameraID camera_;
    Viewport viewport_;

    uint32_t clear_mask_ = 0;

    bool is_active_;

    std::map<DetailLevel, float> detail_level_end_distances_;

    friend class RenderSequence;
};

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

typedef generic::TemplatedManager<Pipeline, PipelineID> PipelineManager;

class RenderSequence:
    public Managed<RenderSequence>,
    public PipelineManager {

public:
    RenderSequence(Window* window);
    ~RenderSequence() {
        delete_all_pipelines();
    }

    PipelinePtr new_pipeline(
        StageID stage,
        CameraID camera,
        const Viewport& viewport=Viewport(),
        TextureID target=TextureID(),
        int32_t priority=0
    );

    PipelinePtr pipeline(PipelineID pipeline);
    void delete_pipeline(PipelineID pipeline);
    void delete_all_pipelines();

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
private:    
    void sort_pipelines(bool acquire_lock=false);
    void run_pipeline(Pipeline::ptr stage, int& actors_rendered);

    Window* window_ = nullptr;
    Renderer* renderer_ = nullptr;

    std::mutex pipeline_lock_;
    std::list<Pipeline::ptr> ordered_pipelines_;

    sig::signal<void (Pipeline&)> signal_pipeline_started_;
    sig::signal<void (Pipeline&)> signal_pipeline_finished_;

    friend class Pipeline;

    std::set<RenderTarget*> targets_rendered_this_frame_;
};

}

#endif // PIPELINE_H
