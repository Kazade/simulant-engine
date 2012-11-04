#include "entity.h"
#include "scene_group.h"

namespace kglt {

SceneGroup::SceneGroup(Scene* parent, SceneGroupID id):
    generic::Identifiable<SceneGroupID>(id),
    entities(this) {

}

}
