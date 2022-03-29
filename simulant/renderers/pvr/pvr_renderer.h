#pragma once

#include "../renderer.h"

namespace smlt {

class Window;

class PVRRenderer:
    public Renderer {

public:
    friend class PVRRenderQueueVisitor;

    PVRRenderer(Window* window);
    ~PVRRenderer() {}

    void init_context() override;

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera);

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
};

}
