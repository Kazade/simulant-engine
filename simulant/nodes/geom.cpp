#include "geom.h"
#include "../stage.h"

namespace smlt {

Geom::Geom(GeomID id, Stage* stage, MeshID mesh, const Vec3 &position, const Quaternion rotation):
    StageNode(stage),
    generic::Identifiable<GeomID>(id),
    Source(stage),
    render_priority_(RENDER_PRIORITY_MAIN) {

    set_parent(stage);

    mesh_ = stage->assets->mesh(mesh)->shared_from_this();

    compile();
}

VertexData* Geom::get_shared_data() const {
    return mesh_->shared_data.get();
}

const AABB Geom::aabb() const {
    return mesh_->aabb();
}

void Geom::ask_owner_for_destruction() {
    stage->delete_geom(id());
}

void Geom::compile() {

}

}
