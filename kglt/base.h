#ifndef BASE_H
#define BASE_H

#include "scene.h"
#include "stage.h"

#include "types.h"

namespace kglt {

class ActorHolder {
public:
    ActorHolder(Scene& scene):
        scene_(scene) {}

    Scene& scene() { return scene_; }

    virtual StageID stage_id() const = 0;
    virtual ActorID actor_id() const = 0;

    Stage* stage() { return &scene_.stage(stage_id()); }
    Actor* actor() { return &stage()->actor(actor_id()); }

private:
    Scene& scene_;
};

}

#endif // BASE_H
