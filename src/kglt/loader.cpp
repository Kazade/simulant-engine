#include "loader.h"
#include "scene.h"

namespace kglt {

Loader::~Loader() {

}

Scene* Loader::loadable_to_scene_ptr(Loadable& resource) {
    Loadable* res_ptr = &resource;
    Scene* scene = dynamic_cast<Scene*>(res_ptr);
    assert(scene && "Tried to cast a loadable that is not a scene... to a scene");    
    return scene;
}

}
