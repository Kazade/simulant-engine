#pragma once

#include <vector>
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

private:
    PVRRenderer* renderer_;
    CameraPtr camera_;

    const MaterialPass* pass_ = nullptr;
    LightPtr light_;

    /* Current PVR list type based on material state */
    int current_list_type_ = 0; /* PVR_LIST_OP_POLY */
    int prev_list_type_ = -1;

#ifdef __DREAMCAST__
    pvr_dr_state_t dr_state_;
#endif

    /* Cached polygon context state */
    bool texturing_enabled_ = false;
    bool depth_test_enabled_ = true;
    bool depth_write_enabled_ = true;
    int cull_mode_ = 0;
    int blend_src_ = 0;
    int blend_dst_ = 0;
    int depth_func_ = 0;
    int shade_mode_ = 0;
    int fog_type_ = 0;

    /* Light state for software lighting */
    struct LightState {
        bool enabled = false;
        float position[4] = {0, 0, 0, 0};
        float color[4] = {1, 1, 1, 1};
        float intensity = 1.0f;
        float range = 100.0f;
    };

    static const int MAX_LIGHTS = 2;
    LightState lights_[MAX_LIGHTS];
    float ambient_[4] = {0.2f, 0.2f, 0.2f, 1.0f};

    /* Material state */
    float mat_diffuse_[4] = {1, 1, 1, 1};
    float mat_ambient_[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    float mat_specular_[4] = {0, 0, 0, 1};
    float mat_shininess_ = 0.0f;

    /* PT and TR renderables are collected during traversal and submitted
     * after all OP geometry, so each PVR list is written exactly once. */
    struct DeferredEntry {
        const Renderable* renderable;
        const MaterialPass* pass;
        int list_type;
        bool texturing_enabled;
        bool depth_test_enabled;
        bool depth_write_enabled;
        int cull_mode, blend_src, blend_dst, depth_func, shade_mode, fog_type;
        float mat_diffuse[4], mat_ambient[4], mat_specular[4];
        float mat_shininess;
        LightState lights[MAX_LIGHTS];
        float ambient[4];
    };

    std::vector<DeferredEntry> deferred_pt_;
    std::vector<DeferredEntry> deferred_tr_;

    void do_visit(const Renderable* renderable,
                  const MaterialPass* material_pass,
                  batcher::Iteration iteration);

    void ensure_list_opened(int list_type);
    void apply_deferred_state(const DeferredEntry& e);
    void flush_deferred(std::vector<DeferredEntry>& entries);
};

} // namespace smlt
