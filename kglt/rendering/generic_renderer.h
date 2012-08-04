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

private:
	void on_start_render();
};

}

#endif // RENDERER_H_INCLUDED
