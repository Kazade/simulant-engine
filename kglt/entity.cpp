#include "scene.h"
#include "entity.h"

namespace kglt {

const VertexData& Entity::shared_data() const {
    return scene().mesh(mesh_).shared_data();
}

Mesh& Entity::_mesh_ref() {
    return scene().mesh(mesh_);
}

void Entity::set_mesh(MeshID mesh) {
    mesh_ = mesh;

    Mesh& m = this->_mesh_ref();

    subentities_.clear();
    for(uint16_t i = 0; i < m.submesh_count(); ++i) {
        subentities_.push_back(SubEntity::create(*this, i));
    }
}

}
