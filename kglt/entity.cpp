#include "scene.h"
#include "entity.h"

namespace kglt {

const VertexData& Entity::shared_data() const {
    return subscene().mesh(mesh_).shared_data();
}

Mesh& Entity::_mesh_ref() {
    return subscene().mesh(mesh_);
}

void Entity::set_mesh(MeshID mesh) {
    mesh_ = mesh;

    Mesh& m = this->_mesh_ref();

    subentities_.clear();
    for(SubMeshIndex idx: m.submesh_ids()) {
        subentities_.push_back(SubEntity::create(*this, idx));
    }

    signal_mesh_changed_(id());
}

void Entity::destroy() {
    subscene().delete_entity(id());
}

}
