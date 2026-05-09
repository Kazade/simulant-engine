#pragma once

#include <vector>
#include <cstdint>
#include "../batching/render_queue.h"

#ifdef __DREAMCAST__
#include <dc/pvr.h>
#endif

namespace smlt {

class PVRRenderer;

class PVRRenderQueueVisitor : public batcher::RenderQueueVisitor {
public:
    PVRRenderQueueVisitor(PVRRenderer* renderer, CameraPtr camera);

    void start_traversal(const batcher::RenderQueue& queue,
                         uint64_t frame_id,
                         StageNode* stage_node) override;

    void visit(const Renderable* renderable, const MaterialPass* pass,
               batcher::Iteration) override;

    void end_traversal(const batcher::RenderQueue& queue, StageNode* stage_node) override;

    void change_render_group(const batcher::RenderGroup* prev,
                             const batcher::RenderGroup* next) override;

    void change_material_pass(const MaterialPass* prev,
                              const MaterialPass* next) override;

    void apply_lights(const LightPtr* lights, const uint8_t count) override;

    struct LightState {
        bool enabled = false;
        float position[4] = {0, 0, 0, 0};
        float dir[3] = {0, 0, -1};  /* Pre-normalized direction (directional lights only) */
        float color[4] = {1, 1, 1, 1};
        float intensity = 1.0f;
        float range = 100.0f;
    };

    static const int MAX_LIGHTS = 2;

private:
    PVRRenderer* renderer_;
    CameraPtr camera_;

    const MaterialPass* pass_ = nullptr;
    LightPtr light_;

#ifdef __DREAMCAST__
    pvr_poly_hdr_t poly_hdr_; /* Compiled polygon header — the current render state */
#endif

    LightState lights_[MAX_LIGHTS];
    float ambient_[4] = {0.2f, 0.2f, 0.2f, 1.0f};

    float mat_diffuse_[4] = {1, 1, 1, 1};
    float mat_ambient_[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    float mat_specular_[4] = {0, 0, 0, 1};
    float mat_shininess_ = 0.0f;

    void do_visit(const Renderable* renderable,
                  const MaterialPass* material_pass,
                  batcher::Iteration iteration);

};

} // namespace smlt
