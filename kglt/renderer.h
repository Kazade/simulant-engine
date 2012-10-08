#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <tr1/memory>

#include "types.h"

namespace kglt {

class Renderer {
public:
    typedef std::tr1::shared_ptr<Renderer> ptr;

    Renderer(Scene& scene):
        scene_(scene) {}

    void render(const std::set<MeshID> meshes); //FIXME: Should pass in batching structure
    Scene& scene() { return scene_; }

private:
    Scene& scene_;

    virtual void render_mesh(const Mesh& mesh) = 0;
};

}

#endif // RENDERER_H
