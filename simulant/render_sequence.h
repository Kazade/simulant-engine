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
    OverlayID overlay_id() { return overlay_; }
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
    void set_overlay(OverlayID s) { overlay_ = s; }
    void set_clear_flags(uint32_t viewport_clear_flags) {
        clear_mask_ = viewport_clear_flags;
    }

    Property<Pipeline, Viewport> viewport = { this, &Pipeline::viewport_ };
private:
    RenderSequence* sequence_;
    int32_t priority_;
    StageID stage_;
    TextureID target_;
    CameraID camera_;
    Viewport viewport_;
    OverlayID overlay_;

    uint32_t clear_mask_ = 0;

    bool is_active_;

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
    RenderSequence(WindowBase* window);
    ~RenderSequence() {
        delete_all_pipelines();
    }

    PipelineID new_pipeline(
        StageID stage,
        CameraID camera,
        const Viewport& viewport=Viewport(),
        TextureID target=TextureID(),
        int32_t priority=0
    );

    PipelineID new_pipeline(
        OverlayID stage,
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

    Property<RenderSequence, WindowBase> window = { this, &RenderSequence::window_ };
private:    
    void sort_pipelines(bool acquire_lock=false);
    void run_pipeline(Pipeline::ptr stage, int& actors_rendered);

    WindowBase* window_ = nullptr;
    Renderer* renderer_ = nullptr;

    std::mutex pipeline_lock_;
    std::list<Pipeline::ptr> ordered_pipelines_;

    sig::signal<void (Pipeline&)> signal_pipeline_started_;
    sig::signal<void (Pipeline&)> signal_pipeline_finished_;

    void update_camera_constraint(CameraID cid);

    friend class Pipeline;

    std::set<RenderTarget*> targets_rendered_this_frame_;
};

}

#endif // PIPELINE_H
