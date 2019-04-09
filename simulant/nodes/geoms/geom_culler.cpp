#include "geom_culler.h"
#include "../../meshes/mesh.h"
#include "../../asset_manager.h"

namespace smlt {

GeomCuller::GeomCuller(Geom *geom, const MeshPtr mesh):
    geom_(geom),
    mesh_(mesh) {

}

GeomCuller::~GeomCuller() {

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

    /* Grab references to materials before releasing the mesh */
    mesh_->each_submesh([this](const std::string, SubMesh* submesh) {
        material_refs_.push_back(mesh_->asset_manager().material(submesh->material_id()));
    });

    // No longer hold onto the mesh, we don't need it anymore
    mesh_.reset();
}

RenderableList GeomCuller::renderables_visible(const Frustum& frustum) {
    RenderableList ret;
    _gather_renderables(frustum, ret);
    return ret;
}

void GeomCuller::each_renderable(EachRenderableCallback cb) {
    RenderableList ret;
    _all_renderables(ret);

    for(auto renderable: ret) {
        cb(renderable.get());
    }
}

}
