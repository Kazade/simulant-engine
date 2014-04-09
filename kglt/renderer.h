#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "generic/protected_ptr.h"
#include "types.h"
#include "utils/geometry_buffer.h"
#include "generic/auto_weakptr.h"
#include "window_base.h"

namespace kglt {

class SubActor;

class Renderer {
public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(WindowBase& window):
        window_(window) {}

    Scene& scene() { return window_.scene(); }

    void set_current_stage(StageID stage) {
        current_stage_ = stage;
    }

    virtual void render_subactor(SubActor& buffer, CameraID camera) = 0;

protected:
    StagePtr current_stage();

private:    
    WindowBase& window_;
    StageID current_stage_;


};

}

#endif // RENDERER_H
