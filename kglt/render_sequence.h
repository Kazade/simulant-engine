#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <memory>
#include <sigc++/sigc++.h>

#include "generic/managed.h"
#include "generic/manager.h"

#include "types.h"
#include "viewport.h"
#include "partitioner.h"
#include "renderer.h"

namespace kglt {


class RenderSequence;

class Pipeline:
    public Managed<Pipeline>,
    public generic::Identifiable<PipelineID>{

public:
    Pipeline(
        RenderSequence* render_sequence,
        PipelineID id
    );

    ViewportID viewport_id() { return viewport_; }
    CameraID camera_id() { return camera_; }
    StageID stage_id() { return stage_; }
    UIStageID ui_stage_id() { return ui_stage_; }

    int32_t priority() const { return priority_; }
    void set_priority(int32_t priority) { priority_ = priority; }

    void deactivate(){ is_active_ = false; }
    void activate() { is_active_ = true; }
    bool is_active() const { return is_active_; }

private:
    void set_stage(StageID s) { stage_ = s; }
    void set_camera(CameraID c) { camera_ = c; }
    void set_viewport(ViewportID v) { viewport_ = v; }
    void set_target(TextureID t) { target_ = t; }
    void set_ui_stage(UIStageID s) { ui_stage_ = s; }

private:
    RenderSequence* sequence_;
    int32_t priority_;
    StageID stage_;
    TextureID target_;
    CameraID camera_;
    ViewportID viewport_;
    UIStageID ui_stage_;

    bool is_active_;

    friend class RenderSequence;
};

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

typedef generic::TemplatedManager<RenderSequence, Pipeline, PipelineID> PipelineManager;

class RenderSequence:
    public Managed<RenderSequence>,
    public PipelineManager {

public:
    RenderSequence(Scene& scene);

    PipelineID new_pipeline(
        StageID stage,
        CameraID camera,
        ViewportID viewport=ViewportID(),
        TextureID target=TextureID(),
        int32_t priority=0
    );

    PipelineID new_pipeline(
        UIStageID stage,
        CameraID camera,
        ViewportID viewport=ViewportID(),
        TextureID target=TextureID(),
        int32_t priority=0
    );

    Pipeline& pipeline(PipelineID pipeline);
    void delete_pipeline(PipelineID pipeline);

    void activate_pipelines(const std::vector<PipelineID>& pipelines);
    std::vector<PipelineID> active_pipelines() const;
    void deactivate_all_pipelines();

    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer::ptr renderer);

    void run();

    sigc::signal<void, Pipeline&>& signal_pipeline_started() { return signal_pipeline_started_; }
    sigc::signal<void, Pipeline&>& signal_pipeline_finished() { return signal_pipeline_finished_; }

    RenderOptions render_options;

private:    
    void run_pipeline(Pipeline::ptr stage);

    Scene& scene_;
    Renderer::ptr renderer_;

    std::list<Pipeline::ptr> ordered_pipelines_;

    sigc::signal<void, Pipeline&> signal_pipeline_started_;
    sigc::signal<void, Pipeline&> signal_pipeline_finished_;

    friend class Scene;
};

}

#endif // PIPELINE_H
