#pragma once

#include "../../assets/material.h"
#include "../batching/renderable.h"

namespace smlt {

struct PVRRenderer;
struct PVRRenderGroupImpl;

struct PVRRenderState {
    Renderable* renderable;
    MaterialPass* pass;
    LightPtr light;
    batcher::Iteration iteration;
    PVRRenderGroupImpl* render_group_impl;
};

class PVRRenderQueueVisitor:
    public batcher::RenderQueueVisitor {

public:
    PVRRenderQueueVisitor(PVRRenderer* renderer, Camera* camera);

    void start_traversal(
        const batcher::RenderQueue& queue,
        uint64_t frame_id,
        Stage* stage
    );

    void visit(
        const Renderable* renderable,
        const MaterialPass* pass,
        batcher::Iteration
    );

    void end_traversal(const batcher::RenderQueue &queue, Stage* stage);

    void change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next);
    void change_material_pass(const MaterialPass* prev, const MaterialPass* next);
    void apply_lights(const LightPtr* lights, const uint8_t count);

private:
    PVRRenderer* renderer_;
    Camera* camera_;
    Colour global_ambient_;

    const MaterialPass* pass_ = nullptr;
    Light* light_ = nullptr;

    PVRRenderGroupImpl* current_group_ = nullptr;

    void do_visit(
        const Renderable* renderable,
        const MaterialPass* material_pass,
        batcher::Iteration iteration
    );

};

}
