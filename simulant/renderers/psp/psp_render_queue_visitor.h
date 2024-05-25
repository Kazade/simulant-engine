#pragma once

#include "../../assets/material.h"
#include "../batching/renderable.h"

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

    void start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage);
    void visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration);
    void end_traversal(const batcher::RenderQueue& queue, Stage* stage);

    void change_render_group(const batcher::RenderGroup* prev, const batcher::RenderGroup* next);
    void change_material_pass(const MaterialPass* prev, const MaterialPass* next);
    void apply_lights(const LightPtr* lights, const uint8_t count);

private:
    PSPRenderer* renderer_;
    CameraPtr camera_;
    Color global_ambient_;

    const MaterialPass* pass_ = nullptr;
    LightPtr light_;

    PSPRenderGroupImpl* current_group_ = nullptr;

    void do_visit(const Renderable* renderable,
                  const MaterialPass* material_pass,
                  batcher::Iteration iteration);
};


}


