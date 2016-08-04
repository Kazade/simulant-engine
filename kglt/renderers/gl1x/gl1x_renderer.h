#pragma once

#include "../renderer.h"

namespace kglt {

class GL1XRenderer : public Renderer {
public:
    GL1XRenderer(WindowBase* window):
        Renderer(window) {}

    batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);

    void render(CameraPtr camera, bool render_group_changed,
        const batcher::RenderGroup *,
        Renderable* renderable,
        MaterialPass* material_pass,
        Light* light,
        const Colour& global_ambient,
        batcher::Iteration iteration
    ) override;

    void init_context();

    void render_triangle_buffers(
        VertexSpecification vertex_format,
        uint8_t* vertex_data,
        const ElementRenderList& render_list
    );
};

}
