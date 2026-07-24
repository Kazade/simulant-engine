#pragma once

#include "../renderer.h"
#include "pvr_texture_manager.h"
#include "../../core/aligned_vector.h"

#ifdef __DREAMCAST__
#include <kos.h>
#endif

/*
 * When enabled OP polys are sent to the PVR via store queues and all other
 * lists are sent over DMA. */
#define HYBRID_RENDERING_ENABLED 0

namespace smlt {

class PVRRenderQueueVisitor;

class PVRRenderer : public Renderer {
public:
    friend class PVRRenderQueueVisitor;

    PVRRenderer(Window* window);
    ~PVRRenderer();

    batcher::RenderGroupKey prepare_render_group(
        batcher::RenderGroup* group, const Renderable* renderable,
        const MaterialPass* material_pass, const RenderPriority priority,
        const uint8_t pass_number, const bool is_blended,
        const float distance_to_camera, uint16_t texture_id) override;

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) override;

    void init_context() override;

    /* Override pre_render to upload textures BEFORE starting the PVR scene.
     * This avoids recursive SQ lock acquisition since both pvr_scene_begin()
     * and pvr_txr_load() use store queues. */
    void pre_render() override;

    std::string name() const override {
        return "pvr";
    }

    bool texture_format_is_native(TextureFormat fmt) override;

    void clear(const RenderTarget& target, const Color& colour, uint32_t clear_flags) override;
    void apply_viewport(const RenderTarget& target,
                        const Viewport& viewport) override;

    std::size_t max_texture_size() const override {
        return 1024;
    }

    void prepare_to_render(const Renderable*) override {}

    PVRTextureManager& texture_manager() { return texture_manager_; }

private:
friend class PVRRenderQueueVisitor;
    PVRTextureManager texture_manager_;

    pvr_list_type_t current_list_type_ = PVR_LIST_OP_POLY;
    pvr_list_type_t prev_list_type_ = (pvr_list_type_t) -1;

    #ifdef __DREAMCAST__
        pvr_dr_state_t dr_state_;

        /* Cache of the PVR's global fog registers (table + color), which the
         * ISP/TSP applies once for the whole scene at render-kick time —
         * unlike blend/depth/cull which are stored per-polygon, there is only
         * one hardware fog config in effect, no matter how many materials
         * with different fog settings are submitted in a frame. Cached here
         * (on the persistent renderer, not the per-frame visitor) so
         * change_material_pass only rebuilds the 129-entry fog table when a
         * material's fog params actually differ from what's already
         * programmed — building it is a real per-call CPU cost, not a cheap
         * register poke. -1 forces the first pass with fog enabled to sync. */
        int32_t fog_mode_cache_ = -1;
        float fog_density_cache_ = -1.0f;
        float fog_start_cache_ = -1.0f;
        float fog_end_cache_ = -1.0f;
        float fog_color_cache_[4] = {-1.0f, -1.0f, -1.0f, -1.0f};

        /* Last pvr_poly_hdr_t bytes submitted to each of the 5 PVR lists
         * (indexed by pvr_list_type_t), so do_visit can skip resubmitting a
         * header that's byte-identical to the one already active for that
         * list — the TA has to redundantly update its internal poly state
         * for a header resend even when nothing about it actually changed.
         * Persists on the renderer (not the per-frame visitor) purely so the
         * array doesn't need reallocating; the *validity* is strictly
         * per-frame — pre_render() clears last_header_valid_ so the first
         * poly submitted to each list this frame always gets its header. */
        pvr_poly_hdr_t last_header_[5];
        bool last_header_valid_[5] = {false, false, false, false, false};
    #endif

    struct ListDMABuffer {
        pvr_list_type_t list_type = PVR_LIST_OP_POLY;
        aligned_vector<uint8_t, 32> buffers[2];
    };

    ListDMABuffer buffers_[4] = {
        { PVR_LIST_OP_MOD, { aligned_vector<uint8_t, 32>(), aligned_vector<uint8_t, 32>() } },
        { PVR_LIST_TR_POLY, { aligned_vector<uint8_t, 32>(), aligned_vector<uint8_t, 32>() } },
        { PVR_LIST_TR_MOD, { aligned_vector<uint8_t, 32>(), aligned_vector<uint8_t, 32>() } },
        { PVR_LIST_PT_POLY, { aligned_vector<uint8_t, 32>(), aligned_vector<uint8_t, 32>() } }
    };

    uint8_t current_buffer_index_ = 0;

    ListDMABuffer& buffer(pvr_list_type_t t) {
        int idx = t;
        idx--;
        return buffers_[idx];
    }

    void ensure_list_opened(pvr_list_type_t list_type);

    void on_pre_render() override;
    void on_post_render() override;
    void do_swap_buffers() override;

    void on_texture_prepare(Texture* texture) override;
    void on_texture_unregister(AssetID tex_id, Texture* texture) override;
    void on_texture_register(AssetID tex_id, Texture* texture) override;

    bool scene_begun_ = false;
    uint32_t clear_colour_packed_ = 0xFF000000;
};

} // namespace smlt
