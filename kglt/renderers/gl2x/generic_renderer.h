#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>

#include "../renderer.h"
#include "../../material.h"

namespace kglt {

class GenericRenderer : public Renderer {
public:
    GenericRenderer(WindowBase* window):
        Renderer(window) {}

    batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);

    void render(CameraPtr camera,
        StagePtr stage, bool render_group_changed,
        const batcher::RenderGroup *,
        Renderable*,
        MaterialPass*,
        Light*,
        batcher::Iteration
    ) override;

private:
    void set_light_uniforms(GPUProgramInstance* program_instance, Light* light);
    void set_material_uniforms(GPUProgramInstance* program_instance, MaterialPass *pass);
    void set_auto_uniforms_on_shader(GPUProgramInstance *pass, CameraPtr camera, Renderable* subactor, const Colour &global_ambient);
    void set_auto_attributes_on_shader(Renderable &buffer);
    void set_blending_mode(BlendType type);
    void send_geometry(Renderable* renderable);
};

}

#endif // GENERIC_RENDERER_H
