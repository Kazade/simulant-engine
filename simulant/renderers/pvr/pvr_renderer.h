#pragma once

#include "../renderer.h"
#include "pvr_texture_manager.h"

#ifdef __DREAMCAST__
#include <kos.h>
#endif

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
        std::vector<uint8_t> pt_buffer_; /* PT headers+vertices pending deferred submission */
        std::vector<uint8_t> tr_buffer_; /* TR headers+vertices pending deferred submission */
    #endif

    void flush_list_buffer(std::vector<uint8_t>& buffer, pvr_list_type_t list_type);
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
