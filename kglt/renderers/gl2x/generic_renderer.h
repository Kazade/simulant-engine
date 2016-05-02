#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>

#include "../renderer.h"
#include "../../utils/geometry_buffer.h"
#include "../../material.h"

namespace kglt {

class GenericRenderer : public Renderer {
public:
    GenericRenderer(WindowBase* window):
        Renderer(window) {}

    new_batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);
private:
    void render(Renderable& mesh, CameraID camera, GPUProgramInstance *program);

    void set_auto_uniforms_on_shader(GPUProgramInstance &pass, CameraID camera, Renderable &subactor);
    void set_auto_attributes_on_shader(Renderable &buffer);
    void set_blending_mode(BlendType type);
};

}

#endif // GENERIC_RENDERER_H
