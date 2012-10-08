#include "renderer.h"
#include "scene.h"

namespace kglt {

void Renderer::render(const std::set<MeshID> meshes) { //FIXME: Should pass in batching structure
    for(MeshID mid: meshes) {
        render_mesh(scene().mesh(mid));
    }
}

}
