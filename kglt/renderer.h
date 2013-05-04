#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <tr1/memory>

#include "types.h"
#include "utils/geometry_buffer.h"

namespace kglt {

class SubEntity;

class Renderer {
public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(Scene& scene):
        scene_(scene) {}

    Scene& scene() { return scene_; }

    void set_current_subscene(SubSceneID subscene) {
        current_subscene_ = subscene;
    }

    virtual void render_subentity(SubEntity& buffer, CameraID camera) = 0;

protected:
    SubScene& current_subscene();

private:    
    Scene& scene_;
    SubSceneID current_subscene_;


};

}

#endif // RENDERER_H
