#include "geom_culler.h"
#include "../../meshes/mesh.h"
#include "../../asset_manager.h"
#include "../../renderers/batching/renderable_store.h"

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

void GeomCuller::renderables_visible(const Frustum& frustum, RenderableFactory* factory) {
    _gather_renderables(frustum, factory);
}

void GeomCuller::each_renderable(EachRenderableCallback cb) {
    RenderableStore store;

    auto factory = store.new_factory();
    _all_renderables(factory);

    factory->each_pushed([&](Renderable* r) {
        cb(r);
    });
}

}
