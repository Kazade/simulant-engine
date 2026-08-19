#pragma once

#include "../../assets/material.h"
#include "../../vertex_data.h"
#include "../batching/renderable.h"
#include "../../utils/vertex_lighting.h"

namespace smlt {

class PSPRenderGroupImpl;
class PSPRenderer;

struct PSPRenderState {
    Renderable* renderable;
    MaterialPass* pass;
    LightPtr light;
    batcher::Iteration iteration;
    PSPRenderGroupImpl* render_group_impl;
};


class PSPRenderQueueVisitor : public batcher::RenderQueueVisitor {
public:
    PSPRenderQueueVisitor(PSPRenderer* renderer, CameraPtr camera);

    void start_traversal(const batcher::RenderQueue &queue,
                         uint64_t frame_id,
                         StageNode *stage_node) override;
    void visit(const Renderable* renderable, const MaterialPass* pass,
               batcher::Iteration) override;
    void end_traversal(const batcher::RenderQueue &queue, StageNode *stage_node) override;

    void change_render_group(const batcher::RenderGroup* prev, const batcher::RenderGroup* next) override;
    void change_material_pass(const MaterialPass* prev, const MaterialPass* next) override;
    void apply_lights(const LightPtr* lights, const uint8_t count) override;

private:
    PSPRenderer* renderer_;
    CameraPtr camera_;

    const MaterialPass* pass_ = nullptr;
    LightPtr light_;

    PSPRenderGroupImpl* current_group_ = nullptr;

    /* Software per-vertex PBR lighting state */
    VertexLightState lights_[4];
    uint8_t light_count_ = 0;
    float ambient_[3] = {0.2f, 0.2f, 0.2f};

    /* PBR material properties */
    float mat_base_color_[4] = {1, 1, 1, 1};
    float mat_metallic_  = 0.0f;
    float mat_roughness_ = 0.4f;
    bool  mat_lighting_enabled_ = false;

    /* Reused scratch buffer that skinned renderables are posed into
     * immediately before submission - see skin_resolve.h. There's no
     * persistent GPU-side buffer on this backend, so a single reused buffer
     * is all that's needed regardless of how many Armature instances share
     * a source mesh. */
    VertexData skin_scratch_{VertexSpecification()};

    void do_visit(const Renderable* renderable,
                  const MaterialPass* material_pass,
                  batcher::Iteration iteration);
};


}


