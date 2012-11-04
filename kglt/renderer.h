#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <tr1/memory>

#include "types.h"
#include "utils/geometry_buffer.h"
#include "entity.h"

namespace kglt {

class Renderer {
public:
    typedef std::tr1::shared_ptr<Renderer> ptr;

    Renderer(Scene& scene):
        scene_(scene) {}

    void render(const std::vector<SubEntity::ptr>& subentities, CameraID camera); //FIXME: Should pass in batching structure
    Scene& scene() { return scene_; }

private:
    Scene& scene_;

    virtual void render_subentity(SubEntity& buffer, CameraID camera) = 0;
};

}

#endif // RENDERER_H
