#include "geom_culler.h"
#include "../../meshes/mesh.h"
#include "../../asset_manager.h"
#include "../../renderers/batching/render_queue.h"
#include "../../renderers/batching/renderable.h"

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

void GeomCuller::compile(const Vec3 &pos, const Quaternion &rot, const Vec3& scale) {
    if(compiled_) {
        // You can only compile once!
        return;
    }

    _compile(pos, rot, scale); // Do whatever the subclass does
    compiled_ = true;

    /* Grab references to materials before releasing the mesh */
    for(auto submesh: mesh_->each_submesh()) {
        material_refs_.push_back(submesh->material());
    }

    /* Release our handle on the mesh */
    mesh_.reset();
}

void GeomCuller::renderables_visible(const Frustum& frustum, batcher::RenderQueue* render_queue) {
    _gather_renderables(frustum, render_queue);
}

void GeomCuller::each_renderable(EachRenderableCallback cb) {
    batcher::RenderQueue queue;

    _all_renderables(&queue);

    for(auto i = 0u; i < queue.renderable_count(); ++i) {
        cb(queue.renderable(i));
    }
}

}
