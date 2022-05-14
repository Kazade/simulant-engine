#pragma once

#include "../renderer.h"

namespace smlt {

class Window;

enum PVRList {
    PVR_LIST_OPAQUE,
    PVR_LIST_OPAQUE_MODIFIER,
    PVR_LIST_PUNCH_THRU,
    PVR_LIST_PUNCH_THRU_MODIFIER,
    PVR_LIST_TRANSPARENT,
    PVR_LIST_TRANSPARENT_MODIFIER
};

struct PVRVertexConfig {
    int type: 3;  /* End of list, tile clip, vertex etc. */
    int end_of_strip: 1;
    int reserved0: 1;
    int list_type: 3; /* Opaque etc. */

    int group_enable: 1; /* Does this block have any effect? */
    int reserved1: 3;
    int strip_length: 2; /* 0==1, 1==2, 3==4, 4==6 */
    int user_clip: 2;  /* 2 == inside, 3 == outside */

    int reserved2: 8;
    int shadow: 1;
    int volume: 1;
    int col_type: 2;
    int texture: 1;
    int offset: 1;
    int gouraud: 1;
    int uv_16bit: 1;
};

struct PVRVertex {
    float x = 0;
    float y = 0;
    float z = 0;
    float u = 0;
    float v = 0;

    uint8_t bgra[4] = {255};
    uint8_t bgra_offset[4] = {0};
};

struct PVRISPInstruction {

};

struct PVRTSPInstruction {

};

struct PVRTextureControl {

};

struct PVRPolyHeader {
    union {
        uint32_t i;
        PVRISPInstruction data;
    } isp;

    union {
        uint32_t i;
        PVRTSPInstruction data;
    } tsp0;

    union {
        uint32_t i;
        PVRTextureControl data;
    } texture0;

    union {
        uint32_t i;
        PVRTSPInstruction data;
    } tsp1;

    union {
        uint32_t i;
        PVRTextureControl data;
    } texture1;

    /* These values need to be set if we're using sort-DMA */
    uint32_t data_size = 0xffffffff;
    uint32_t next_address = 0xffffffff;
};

struct PVRCommand {
    union {
        uint32_t i;
        PVRVertexConfig vc;
    } control = {0xE000000A};  /* Set the default to be a vertex, but not EOL with textures and gouraud shading */

    union {
        PVRPolyHeader poly;
        PVRVertex vertex;
    } data;
};

class PVRRenderer:
    public Renderer {

public:
    friend class PVRRenderQueueVisitor;

    PVRRenderer(Window* window);
    ~PVRRenderer() {}

    void init_context() override;

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) override;

    batcher::RenderGroupKey prepare_render_group(
        batcher::RenderGroup* group,
        const Renderable *renderable,
        const MaterialPass *material_pass,
        const uint8_t pass_number,
        const bool is_blended,
        const float distance_to_camera
    ) override;

    std::string name() const override {
        return "PVR";
    }

    void prepare_to_render(const Renderable* renderable) override;

private:
    void on_texture_register(TextureID tex_id, Texture* texture) override;
    void on_texture_unregister(TextureID tex_id, Texture* texture) override;
    void on_texture_prepare(Texture* texture) override;
    void on_material_prepare(Material* material) override;

    /* PVR specific stuff */
    PVRList current_list_ = PVR_LIST_OPAQUE;
    void on_list_change(PVRList new_list);

    std::vector<PVRCommand> command_buffer_;

    /* Submit the command buffer to the PVR and then wipe the buffer.
     * FIXME: Double buffer and submit in a background thread? */
    void flush_buffer();
};

}
