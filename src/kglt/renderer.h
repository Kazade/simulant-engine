#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "glee/GLee.h"
#include <iostream>

#include "object.h"
#include "object_visitor.h"

namespace kglt {

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

    void start_render(Scene* scene);
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
