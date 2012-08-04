#ifndef KGLT_GENERIC_RENDERER_H_INCLUDED
#define KGLT_GENERIC_RENDERER_H_INCLUDED

#include <iostream>

#include "kglt/renderer.h"

namespace kglt {

class Mesh;
class Camera;
class Scene;
class Text;

class GenericRenderer : public Renderer {
public:
	typedef std::tr1::shared_ptr<Renderer> ptr;

    GenericRenderer(const RenderOptions& options=RenderOptions()):
        Renderer(options) {}
            
    void visit(Mesh& mesh);
    void visit(Text& text);
    void visit(Background& background);

private:    
    void on_start_render(Scene& scene);

    void render_mesh(Mesh& mesh, Scene& scene);
};

}

#endif // RENDERER_H_INCLUDED
