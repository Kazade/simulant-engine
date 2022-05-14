#include "pvr_renderer.h"
#include "pvr_render_queue_visitor.h"

/* This value is from GLdc, which got it from libgl - need to determine if this
 * is bytes or vertices and make it clearer (e.g. what's the 256?) */
#define PVR_VERTEX_BUF_SIZE (2560 * 256)

namespace smlt {

PVRRenderer::PVRRenderer(Window* window):
    Renderer(window) {

}

void PVRRenderer::init_context() {
    pvr_init_params_t params = {
        {PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32},
        PVR_VERTEX_BUF_SIZE, /* Vertex buffer size */
        0, /* No DMA */
        0, /* No FSAA */
        1 /* Disable translucent auto-sorting to match traditional GL */
    };

    pvr_init(&params);
}

std::shared_ptr<batcher::RenderQueueVisitor> PVRRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PVRRenderQueueVisitor>(this, camera);
}

batcher::RenderGroupKey PVRRenderer::prepare_render_group(batcher::RenderGroup* group, const Renderable* renderable, const MaterialPass* material_pass, const uint8_t pass_number, const bool is_blended, const float distance_to_camera) {
    return batcher::RenderGroupKey();
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
