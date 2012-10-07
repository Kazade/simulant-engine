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
    typedef std::tr1::shared_ptr<SceneGroup> ptr;

    ReverseRelation<SceneGroup, Mesh> meshes;

    SceneGroup(Scene& parent);

private:
    uint32_t priority_;
};


}

#endif
