#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <tr1/memory>
#include <sigc++/sigc++.h>

#include "generic/managed.h"

#include "types.h"
#include "viewport.h"
#include "partitioner.h"
#include "renderer.h"

namespace kglt {

class PipelineStage:
    public Managed<PipelineStage> {

public:
    ViewportID viewport_id() { return viewport_; }
    CameraID camera_id() { return camera_; }
    StageID stage_id() { return stage_; }

    int32_t priority() const { return priority_; }
    void set_priority(int32_t priority) { priority_ = priority; }

private:
    PipelineStage(Scene& scene, StageID ss, CameraID camera, ViewportID viewport, TextureID target);

private:
    int32_t priority_;
    Scene& scene_;
    StageID stage_;
    TextureID target_;
    CameraID camera_;

    ViewportID viewport_;

    friend class Pipeline;
};

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class Pipeline:
    public Managed<Pipeline> {

public:
    Pipeline(Scene& scene);

    void remove_all_stages();
    void add_stage(StageID stage, CameraID camera, ViewportID viewport=ViewportID(), TextureID target=TextureID(), int32_t priority=0);

    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer::ptr renderer);

    void run();

    sigc::signal<void, PipelineStage&>& signal_render_stage_started() { return signal_render_stage_started_; }
    sigc::signal<void, PipelineStage&>& signal_render_stage_finished() { return signal_render_stage_finished_; }

    RenderOptions render_options;

private:    
    void run_stage(PipelineStage::ptr stage);

    Scene& scene_;

    Renderer::ptr renderer_;

    std::vector<PipelineStage::ptr> stages_;

    sigc::signal<void, PipelineStage&> signal_render_stage_started_;
    sigc::signal<void, PipelineStage&> signal_render_stage_finished_;

    friend class Scene;
};

}

#endif // PIPELINE_H
