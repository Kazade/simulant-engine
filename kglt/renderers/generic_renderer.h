#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>

#include "../utils/geometry_buffer.h"
#include "../renderer.h"
#include "../material.h"

namespace kglt {

class GenericRenderer : public Renderer {
public:
    GenericRenderer(Scene& scene):
        Renderer(scene) {}

private:
    void render_subentity(SubEntity& mesh, CameraID camera);

    void set_auto_uniforms_on_shader(ShaderProgram& s, Scene& scene,
        const std::vector<LightID>& lights_within_range, uint32_t iteration, CameraID camera, SubEntity &subentity);

    void set_auto_attributes_on_shader(ShaderProgram& s, SubEntity &buffer);
    void set_blending_mode(BlendType type);
};

}

#endif // GENERIC_RENDERER_H
