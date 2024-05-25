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

    void start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id,
                         Stage* stage) override;
    void visit(const Renderable* renderable, const MaterialPass* pass,
               batcher::Iteration) override;
    void end_traversal(const batcher::RenderQueue& queue,
                       Stage* stage) override;

    void change_render_group(const batcher::RenderGroup* prev,
                             const batcher::RenderGroup* next) override;
    void change_material_pass(const MaterialPass* prev,
                              const MaterialPass* next) override;
    void apply_lights(const LightPtr* lights, const uint8_t count) override;

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


