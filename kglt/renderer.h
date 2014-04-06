#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "generic/protected_ptr.h"
#include "types.h"
#include "utils/geometry_buffer.h"
#include "generic/auto_weakptr.h"

namespace kglt {

class SubActor;

class Renderer {
public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(Scene& scene):
        scene_(scene) {}

    Scene& scene() { return scene_; }

    void set_current_stage(StageID stage) {
        current_stage_ = stage;
    }

    virtual void render_subactor(SubActor& buffer, CameraID camera) = 0;

protected:
    AutoWeakPtr<Stage> current_stage();

private:    
    Scene& scene_;
    StageID current_stage_;


};

}

#endif // RENDERER_H
