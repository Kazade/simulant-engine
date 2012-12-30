#include "renderer.h"
#include "scene.h"

namespace kglt {

SubScene& Renderer::current_subscene() { return scene().subscene(current_subscene_); }

void Renderer::render(const std::vector<SubEntity::ptr>& subentities, CameraID camera) { //FIXME: Should pass in batching structure
    for(SubEntity::ptr subentity: subentities) {
        render_subentity(*subentity, camera);
    }
}

}
