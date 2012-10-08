#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>

#include "../renderer.h"

namespace kglt {

class GenericRenderer : public Renderer {
public:
    GenericRenderer(Scene& scene):
        Renderer(scene) {}

private:
    void render_mesh(const Mesh &mesh);

    void set_auto_uniforms_on_shader(ShaderProgram& s, Scene& scene,
        const std::vector<LightID>& lights_within_range, uint32_t iteration);

    void set_auto_attributes_on_shader(ShaderProgram& s);
};

}

#endif // GENERIC_RENDERER_H
