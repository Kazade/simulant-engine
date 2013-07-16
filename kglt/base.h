#ifndef BASE_H
#define BASE_H

#include "types.h"

namespace kglt {

class ActorHolder {
public:
    ActorHolder(Scene& scene):
        scene_(scene) {}

    Scene& scene() { return scene_; }

    virtual StageID stage_id() const = 0;
    virtual ActorID actor_id() const = 0;

private:
    Scene& scene_;
};

}

#endif // BASE_H
