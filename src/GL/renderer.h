#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include <iostream>
#include <SDL_opengl.h>

#include "object.h"
#include "object_visitor.h"

namespace GL {

class Mesh;
class Camera;
class Scene;

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class Renderer : public ObjectVisitor {
public:
    Renderer(RenderOptions options):
        options_(options) {}

    ~Renderer();

    void start_render(Scene* scene) {
        scene_ = scene;

        if(!options_.texture_enabled) {
            glDisable(GL_TEXTURE_2D);
        } else {
            glEnable(GL_TEXTURE_2D);
        }

        if(options_.wireframe_enabled) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if(!options_.backface_culling_enabled) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
        }

        glPointSize(options_.point_size);
    }

    void visit(Mesh* mesh);

    //The default camera is set in start_render
    //TODO: Add debug mode which renders camera positions
    void visit(Camera* camera) {}

    //We don't really care about the Scene object, but it is an Object
    //so this method must exist
    void visit(Scene* scene) {}

    void finish_render() {}

private:
    RenderOptions options_;
    Scene* scene_;
};

}

#endif // RENDERER_H_INCLUDED
