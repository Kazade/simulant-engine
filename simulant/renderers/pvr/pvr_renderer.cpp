#include "pvr_renderer.h"
#include "pvr_render_queue_visitor.h"

namespace smlt {

PVRRenderer::PVRRenderer(Window* window):
    Renderer(window) {

}

void PVRRenderer::init_context() {

}

std::shared_ptr<batcher::RenderQueueVisitor> PVRRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PVRRenderQueueVisitor>(this, camera);
}

batcher::RenderGroupKey PVRRenderer::prepare_render_group(batcher::RenderGroup* group, const Renderable* renderable, const MaterialPass* material_pass, const uint8_t pass_number, const bool is_blended, const float distance_to_camera) {

}

void PVRRenderer::prepare_to_render(const Renderable* renderable) {

}

void PVRRenderer::on_texture_register(TextureID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);
    _S_UNUSED(texture);
}

void PVRRenderer::on_texture_unregister(TextureID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);
    _S_UNUSED(texture);
}

void PVRRenderer::on_texture_prepare(Texture* texture) {
    _S_UNUSED(texture);
}

void PVRRenderer::on_material_prepare(Material* material) {
    _S_UNUSED(material);
}

}
