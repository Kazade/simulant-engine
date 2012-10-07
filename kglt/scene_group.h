#ifndef KGLT_SCENE_GROUP_H
#define KGLT_SCENE_GROUP_H

#include "generic/relation.h"
#include "generic/identifiable.h"
#include "types.h"

namespace kglt {

class SceneGroup :
    public generic::Identifiable<SceneGroupID>,
    public Relatable {

public:
    ReverseRelation<SceneGroup, Mesh> meshes;

    SceneGroup(Scene& parent, uint32_t priority);
    uint32_t priority() const { return priority_; }

private:
    uint32_t priority_;
};


}

#endif
