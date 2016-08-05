#include "geom.h"
#include "stage.h"

namespace kglt {

Geom::Geom(GeomID id, Stage* stage, MeshID mesh, const Vec3 &position, const Quaternion rotation):
    generic::Identifiable<GeomID>(id),
    Object(stage),
    Source(stage),
    render_priority_(RENDER_PRIORITY_MAIN) {

    set_parent(stage);
    set_mesh(mesh);
}

VertexData* Geom::get_shared_data() const {
    return mesh_->shared_data.get();
}

void Geom::set_mesh(MeshID mesh) {
    //Increment the ref-count on this mesh
    mesh_ = stage->resources->mesh(mesh).__object;

    // Tell the partitioner that stuff changed
    signal_mesh_changed_(id());
}

const AABB Geom::aabb() const {
    return mesh_->aabb();
}

const AABB Geom::transformed_aabb() const {
    AABB box = aabb(); //Get the untransformed one

    auto pos = absolute_position();
    kmVec3Add(&box.min, &box.min, &pos);
    kmVec3Add(&box.max, &box.max, &pos);
    return box;
}

void Geom::ask_owner_for_destruction() {
    stage->delete_geom(id());
}


}
