#include "geom_culler.h"

namespace smlt {

GeomCuller::GeomCuller(Geom *geom, const MeshPtr mesh):
    geom_(geom),
    mesh_(mesh) {

}

bool GeomCuller::is_compiled() const {
    return compiled_;
}

void GeomCuller::compile() {
    if(compiled_) {
        // You can only compile once!
        return;
    }

    _compile(); // Do whatever the subclass does

    compiled_ = true;

    // No longer hold onto the mesh, we don't need it anymore
    mesh_.reset();
}

RenderableList GeomCuller::renderables_visible(const Frustum& frustum) {
    RenderableList ret;
    _gather_renderables(frustum, ret);
    return ret;
}


}
