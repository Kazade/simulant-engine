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

	GenericRenderer(Scene& scene):
		Renderer(scene) {}
		
	GenericRenderer(Scene& scene, const RenderOptions& options):
        Renderer(scene, options) {}
            
    void visit(Mesh* mesh);

    //The default camera is set in start_render
    //TODO: Add debug mode which renders camera positions
    void visit(Camera* camera) {}

    //We don't really care about the Scene object, but it is an Object
    //so this method must exist
    void visit(Scene* scene) {}

    //Sprites are just containers
    void visit(Sprite* sprite) {}	  
    
    //Backgrounds and Layers are just containers
    void visit(Background* bg) {}
    void visit(BackgroundLayer* layer) {}
    void visit(UI* ui) {}
    void visit(ui::Element* element) {}
    void visit(Overlay* overlay) {}

    //Text is magic, we need to handle it
    void visit(Text* text);

private:
	void on_start_render();
};

}

#endif // RENDERER_H_INCLUDED
