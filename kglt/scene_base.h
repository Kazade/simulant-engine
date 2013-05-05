#ifndef SCENE_BASE_H
#define SCENE_BASE_H

#include "resource_manager.h"

namespace kglt {

class SceneBase:
    public ResourceManager {
public:
    SceneBase(WindowBase* window, ResourceManager* parent=nullptr):
        ResourceManager(window, parent) {}

    virtual ~SceneBase() {}
};


}

#endif // SCENE_BASE_H
