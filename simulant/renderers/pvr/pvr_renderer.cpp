#include "pvr_renderer.h"
#include "pvr_render_queue_visitor.h"
#include "../../window.h"

#ifdef __DREAMCAST__
#include <kos.h>
#include <dc/pvr.h>
#include <dc/video.h>
#endif

namespace smlt {

PVRRenderer::PVRRenderer(Window* window):
    Renderer(window),
    texture_manager_(this) {

}

PVRRenderer::~PVRRenderer() {

}

batcher::RenderGroupKey PVRRenderer::prepare_render_group(
    batcher::RenderGroup* group,
    const Renderable* renderable,
    const MaterialPass* material_pass,
    const RenderPriority priority,
    const uint8_t pass_number,
    const bool is_blended,
    const float distance_to_camera,
    uint16_t texture_id) {

    _S_UNUSED(renderable);
    _S_UNUSED(material_pass);
    _S_UNUSED(group);

    return batcher::generate_render_group_key(
        priority, pass_number, is_blended, distance_to_camera,
        renderable->precedence, texture_id);
}

std::shared_ptr<batcher::RenderQueueVisitor> PVRRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PVRRenderQueueVisitor>(this, camera);
}

void PVRRenderer::init_context() {
#ifdef __DREAMCAST__
    S_INFO("Initializing PVR direct rendering context");

    /* PVR should already be initialized by the KOS window init
     * via pvr_init(). If not, we do it here. */
    pvr_init_params_t params = {
        /* Bin sizes for: OP, OP_MOD, TR, TR_MOD, PT */
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 },
        512 * 1024, /* Vertex buffer size */
        0,          /* DMA enabled */
        0,          /* FSAA */
        0           /* Autosort (0 = enabled for TR) */
    };

    if(pvr_init(&params) < 0) {
        S_ERROR("PVR init failed! May already be initialized.");
    }

    /* Set default background color to black */
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

    S_INFO("PVR direct rendering context initialized");
    S_INFO("PVR VRAM available: {0} bytes", pvr_mem_available());
#endif
}

bool PVRRenderer::texture_format_is_native(TextureFormat fmt) {
    switch(fmt) {
        /* Standard 16-bit packed formats - PVR can handle non-twiddled too */
        case TEXTURE_FORMAT_RGB_1US_565:
        case TEXTURE_FORMAT_RGBA_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_1555:
        case TEXTURE_FORMAT_ARGB_1US_4444:
        /* Twiddled formats - native and fast */
        case TEXTURE_FORMAT_RGB_1US_565_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID:
        /* VQ compressed twiddled - native and fastest */
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
        /* VQ with mipmaps */
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
            return true;
        default:
            return false;
    }
}

void PVRRenderer::clear(const RenderTarget& target, const Color& colour, uint32_t clear_flags) {
    _S_UNUSED(target);
    _S_UNUSED(clear_flags);
#ifdef __DREAMCAST__
    pvr_set_bg_color(colour.r, colour.g, colour.b);
    clear_colour_packed_ = colour.to_argb_8888();
#else
    _S_UNUSED(colour);
#endif
}

void PVRRenderer::apply_viewport(const RenderTarget& target, const Viewport& viewport) {
    _S_UNUSED(target);
    _S_UNUSED(viewport);
    /* PVR doesn't support viewport transformation in hardware in the same
     * way as OpenGL. We handle viewport in the projection matrix. */
}

void PVRRenderer::on_pre_render() {
#ifdef __DREAMCAST__
    texture_manager_.update_priorities();
    pvr_wait_ready();
    pvr_scene_begin();
    scene_begun_ = true;
#endif
}

void PVRRenderer::on_post_render() {
#ifdef __DREAMCAST__
    if(scene_begun_) {
        pvr_scene_finish();
        scene_begun_ = false;
    }
#endif
}

void PVRRenderer::do_swap_buffers() {
    /* PVR handles double buffering internally via pvr_scene_begin/finish */
}

void PVRRenderer::on_texture_register(AssetID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);
    /* Assign a unique renderer ID. The texture manager will handle VRAM. */
    static uint32_t next_id = 1;
    texture->_set_renderer_specific_id(next_id++);
}

void PVRRenderer::on_texture_unregister(AssetID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);
    uint32_t id = texture->_renderer_specific_id();
    if(id) {
        texture_manager_.release_texture(id);
        texture->_set_renderer_specific_id(0);
    }
}

void PVRRenderer::on_texture_prepare(Texture* texture) {
    if(!texture->_data_dirty() && !texture->_params_dirty()) {
        return;
    }

    if(!texture->_data_dirty() || !texture->auto_upload()) {
        if(texture->_params_dirty()) {
            /* Update filter/wrap params on the existing texture */
            auto obj = texture_manager_.find_texture(texture->_renderer_specific_id());
            if(obj) {
                obj->filter = texture->texture_filter();
            }
            texture->_set_params_clean();
        }
        return;
    }

    uint32_t id = texture->_renderer_specific_id();
    if(!id) return;

    auto w = texture->width();
    auto h = texture->height();
    auto data = texture->data();
    auto data_size = texture->data_size();

    if(!data || data_size == 0) {
        return;
    }

#ifdef __DREAMCAST__
    int pvr_format = -1;
    bool is_compressed = false;
    bool is_twiddled = false;
    bool has_mipmaps = false;

    switch(texture->format()) {
        case TEXTURE_FORMAT_RGB_1US_565:
            pvr_format = PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED;
            break;
        case TEXTURE_FORMAT_ARGB_1US_4444:
            pvr_format = PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED;
            break;
        case TEXTURE_FORMAT_ARGB_1US_1555:
            pvr_format = PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED;
            break;
        case TEXTURE_FORMAT_RGB_1US_565_TWID:
            pvr_format = PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED;
            is_twiddled = true;
            break;
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
            pvr_format = PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_TWIDDLED;
            is_twiddled = true;
            break;
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID:
            pvr_format = PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED;
            is_twiddled = true;
            break;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
            pvr_format = PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
            is_compressed = true;
            is_twiddled = true;
            break;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
            pvr_format = PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
            is_compressed = true;
            is_twiddled = true;
            break;
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
            pvr_format = PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
            is_compressed = true;
            is_twiddled = true;
            break;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
            pvr_format = PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
            is_compressed = true;
            is_twiddled = true;
            has_mipmaps = true;
            break;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
            pvr_format = PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
            is_compressed = true;
            is_twiddled = true;
            has_mipmaps = true;
            break;
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
            pvr_format = PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
            is_compressed = true;
            is_twiddled = true;
            has_mipmaps = true;
            break;
        case TEXTURE_FORMAT_RGBA_1US_4444:
            /* Need to swizzle RGBA4444 to ARGB4444 */
            pvr_format = PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED;
            break;
        default:
            S_WARN("Unsupported texture format for PVR: {0}", (int)texture->format());
            return;
    }

    texture_manager_.upload_texture(
        id, pvr_format, w, h,
        data_size, data,
        is_compressed, is_twiddled, has_mipmaps,
        texture->texture_filter()
    );

#endif

    texture->_set_data_clean();
    texture->_set_params_clean();

    /* Free CPU-side data if requested */
    if(texture->free_data_mode() == TEXTURE_FREE_DATA_AFTER_UPLOAD) {
        texture->free();
    }
}

} // namespace smlt