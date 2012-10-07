#ifndef KGLT_GENERIC_RENDERER_H_INCLUDED
#define KGLT_GENERIC_RENDERER_H_INCLUDED

#include <iostream>

#include "../renderer.h"
#include "../generic/creator.h"
#include "../types.h"

namespace kglt {

class Mesh;
class Camera;
class Scene;
class Text;

class GenericRenderer :
    public Renderer,
    public generic::Creator<GenericRenderer> {
public:
	typedef std::tr1::shared_ptr<Renderer> ptr;

    GenericRenderer(const RenderOptions& options=RenderOptions()):
        Renderer(options) {}
            
    void visit(Mesh& mesh);
    void visit(Text& text);
    void visit(Background& background);

    void _initialize(Scene& scene);

private:    
    void on_start_render(Scene& scene);
    void render_mesh(Mesh& mesh, Scene& scene);

    void set_auto_uniforms_on_shader(
        ShaderProgram& shader,
        Scene& scene,
        const std::vector<LightID>& lights_within_range,
        uint32_t iteration
    );
    void set_auto_attributes_on_shader(ShaderProgram& shader);
};

}

#endif // RENDERER_H_INCLUDED
