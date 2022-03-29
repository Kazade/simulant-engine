#include "pvr_render_queue_visitor.h"

namespace smlt {

PVRRenderQueueVisitor::PVRRenderQueueVisitor(PVRRenderer* renderer, Camera* camera):
    renderer_(renderer),
    camera_(camera) {

}

void PVRRenderQueueVisitor::start_traversal(
    const batcher::RenderQueue& queue,
    uint64_t frame_id,
    Stage* stage
) {

}

void PVRRenderQueueVisitor::visit(
    const Renderable* renderable,
    const MaterialPass* pass,
    batcher::Iteration
) {

}

void PVRRenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, Stage* stage) {

}

void PVRRenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {

}

void PVRRenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {

}

void PVRRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {

}


}
