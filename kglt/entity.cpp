#include "scene.h"
#include "entity.h"

namespace kglt {

const VertexData& Entity::shared_data() const {
    return scene().newmesh(mesh_).shared_data();
}

newmesh::Mesh& Entity::_mesh_ref() {
    return scene().newmesh(mesh_);
}

void Entity::set_mesh(newmesh::MeshID mesh) {
    mesh_ = mesh;

    newmesh::Mesh& m = this->_mesh_ref();

    subentities_.clear();
    for(uint16_t i = 0; i < m.submesh_count(); ++i) {
        subentities_.push_back(SubEntity::create(*this, i));
    }
}

}
