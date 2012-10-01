#ifndef KGLT_SCENE_GROUP_H
#define KGLT_SCENE_GROUP_H

#include "generic/identifiable.h"

namespace kglt {

class SceneGroup :
    public generic::Identifiable<SceneGroupID> {

public:
    SceneGroup(Scene& parent, uint32_t priority);
    void add_pass(CameraID camera_id, ViewportType viewport, TextureID target=0);

    uint32_t priority() const { return priority_; }

private:
    uint32_t priority_;
};


}

#endif
