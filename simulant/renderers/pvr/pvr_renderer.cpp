#include "pvr_renderer.h"
#include "pvr_render_queue_visitor.h"
#include "../../window.h"

#ifdef __DREAMCAST__
#include <kos.h>
#include <dc/pvr.h>
#include <dc/video.h>

extern struct pvr_state_t pvr_state;
#endif

namespace smlt {

PVRRenderer::PVRRenderer(Window* window):
    Renderer(window),
    texture_manager_(this) {

    memset(&dr_state_, 0, sizeof(dr_state_));
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
        /* Bin sizes for: OP, OP_MOD, TR, TR_MOD, PT.
         * OP_MOD is bumped from BINSIZE_0 so cheap-shadow modifier volumes
         * (submitted by ShadowCaster) have somewhere to land. */
        { PVR_BINSIZE_32, PVR_BINSIZE_32, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32 },
        512 * 1024, /* Vertex buffer size */
        1,          /* DMA enabled */
        0,          /* FSAA */
        0,          /* Autosort (0 = enabled for TR) */
        8,          /* OPB overflow count */
        0           /* VBUF doublebuf disabled */
    };

    if(pvr_init(&params) < 0) {
        S_ERROR("PVR init failed! May already be initialized.");
    }

    /* Set default background color to black */
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

    /* Enable cheap-shadow mode. Modifier-enabled receiver polygons whose
     * pixels fall inside a submitted modifier volume are darkened by this
     * scale factor (smaller = darker shadow). */
    pvr_set_shadow_scale(true, 0.5f);

    S_VERBOSE("PVR direct rendering context initialized");
    S_VERBOSE("PVR VRAM available: {0} bytes", pvr_mem_available());
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

void PVRRenderer::pre_render() {
    /* Prepare textures BEFORE starting the PVR scene. Both pvr_scene_begin()
     * and pvr_txr_load() use store queues, so we must not nest them. */
    for(auto& wptr: _texture_registry()) {
        prepare_texture(wptr.second);
    }

#ifdef __DREAMCAST__
    texture_manager_.update_priorities();

    S_VERBOSE("Beginning scene");
    pvr_scene_begin();
    scene_begun_ = true;

    prev_list_type_ = (pvr_list_type_t) -1;
    current_list_type_ = PVR_LIST_OP_POLY;

    ensure_list_opened(current_list_type_);
#endif
}

void PVRRenderer::on_pre_render() {
    /* Textures are now prepared in pre_render(). Nothing to do here. */
}

/* ========================================================================
 * ensure_list_opened - open a PVR list if not already open
 * ======================================================================== */

void PVRRenderer::ensure_list_opened(pvr_list_type_t list_type) {
#ifdef __DREAMCAST__
    if(list_type == prev_list_type_) return;

    if(prev_list_type_ >= 0) {
        S_VERBOSE("Finishing list {0}", prev_list_type_);
        pvr_list_finish();
    }

    S_VERBOSE("Beginning list {0}", list_type);
    pvr_list_begin(list_type);
    prev_list_type_ = list_type;
#else
    _S_UNUSED(list_type);
#endif
}

void PVRRenderer::on_post_render() {

    // pvr_state is internal, ram_target is 9 uint32_t things in
    uint32_t* ram_target = ((uint32_t*) &pvr_state) + 9;

#ifdef __DREAMCAST__
    if(scene_begun_) {
        pvr_list_finish();
        prev_list_type_ = (pvr_list_type_t) -1;

        for(auto list_type: { PVR_LIST_OP_MOD, PVR_LIST_PT_POLY, PVR_LIST_TR_POLY }) {
            auto& buf = buffer(list_type);
            auto& b = buf.buffers[current_buffer_index_];
            auto count = b.size();

            /* Reserve room for KOS's 32-byte zero terminator (appended
             * in pvr_scene_finish), then round up to 64. */
            size_t buf_size = (count + 32 + 63) & ~63;
            if(buf_size < 64) buf_size = 64;
            b.resize(buf_size);

            /* This is a hack! KOS assumes we're using the same buffer for both
             * front and back ram targets. We instead use two different dynamic buffers so we:
             *
             * 1. Tell KOS the buffers are twice the size they are (kos halves it)
             * 2. force the ram target to 0 (first half)
             */

            *ram_target = 0;  // HACK
            pvr_set_vertbuf(buf.list_type, &b[0], b.size() * 2);
            pvr_vertbuf_written(buf.list_type, count);
        }

        S_VERBOSE("Finishing scene");

        pvr_scene_finish();

        for(auto& buf: buffers_) {
            buf.buffers[current_buffer_index_].clear();
        }

        scene_begun_ = false;
    }
#endif

    current_buffer_index_ = (current_buffer_index_ + 1) % 2;
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
