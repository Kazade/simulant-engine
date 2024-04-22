#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>

#include "psp_render_queue_visitor.h"
#include "psp_renderer.h"
#include "psp_render_group_impl.h"

#include "../../stage.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../utils/gl_error.h"
#include "../../window.h"
#include "../../application.h"

namespace smlt {

PSPRenderQueueVisitor::PSPRenderQueueVisitor(PSPRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void PSPRenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue,
                                            uint64_t frame_id, Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);

    sceGuStart(GU_DIRECT, (void*) renderer_->display_list());
    sceGuClearColor(0xFFFFFFFF); // White background
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

void PSPRenderQueueVisitor::visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration iteration) {
    do_visit(renderable, pass, iteration);
}

void PSPRenderQueueVisitor::end_traversal(const batcher::RenderQueue& queue,
                                          Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(stage);

    sceGuFinish();
}

void PSPRenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

void PSPRenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    pass_ = next;
}

void PSPRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    if(!count) {
        return;
    }
}

void PSPRenderQueueVisitor::do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    _S_UNUSED(material_pass);
    _S_UNUSED(iteration);

}

}


