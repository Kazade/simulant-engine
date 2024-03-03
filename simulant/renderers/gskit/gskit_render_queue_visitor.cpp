
#include "gskit_render_queue_visitor.h"
#include "gskit_renderer.h"
#include "gskit_render_group_impl.h"

#include "../../stage.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../utils/gl_error.h"
#include "../../window.h"
#include "../../application.h"

namespace smlt {

GSKitRenderQueueVisitor::GSKitRenderQueueVisitor(GSKitRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void GSKitRenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, StageNode* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);
}

void GSKitRenderQueueVisitor::visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration iteration) {
    do_visit(renderable, pass, iteration);
}

void GSKitRenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, StageNode *stage) {
    _S_UNUSED(queue);
    _S_UNUSED(stage);
}

void GSKitRenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

void GSKitRenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    pass_ = next;
}

void GSKitRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    if(!count) {
        return;
    }
}

void GSKitRenderQueueVisitor::do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    _S_UNUSED(material_pass);
    _S_UNUSED(iteration);

}

}
