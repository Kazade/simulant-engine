#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <tr1/memory>
#include <sigc++/sigc++.h>

#include "types.h"
#include "viewport.h"
#include "partitioner.h"
#include "renderer.h"

namespace kglt {

class Pass {
public:
    typedef std::tr1::shared_ptr<Pass> ptr;

private:
    Pass(Scene& scene, SceneGroupID sg, TextureID target, CameraID camera, ViewportType viewport);

private:
    Scene& scene_;
    SceneGroupID scene_group_;
    TextureID target_;
    CameraID camera_;

    Viewport viewport_;

    friend class Pipeline;
};

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class Pipeline {
public:
    typedef std::tr1::shared_ptr<Pipeline> ptr;

    Pipeline(Scene& scene);

    void remove_all_passes();
    void add_pass(SceneGroupID scene_group, TextureID target=0, CameraID camera=0, ViewportType viewport=VIEWPORT_TYPE_FULL);

    void set_partitioner(Partitioner::ptr partitioner);
    //void set_batcher(Batcher::ptr batcher);
    void set_renderer(Renderer::ptr renderer);

    void run();
    void run_pass(uint32_t index);

    sigc::signal<void, uint32_t>& signal_render_pass_started() { return signal_render_pass_started_; }
    sigc::signal<void, uint32_t>& signal_render_pass_finished() { return signal_render_pass_finished_; }

    RenderOptions render_options;
private:
    Scene& scene_;
    Partitioner::ptr partitioner_;
    Renderer::ptr renderer_;

    std::vector<Pass::ptr> passes_;

    sigc::signal<void, uint32_t> signal_render_pass_started_;
    sigc::signal<void, uint32_t> signal_render_pass_finished_;
};

}

#endif // PIPELINE_H
