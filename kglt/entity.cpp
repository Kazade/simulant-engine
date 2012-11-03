#include "scene.h"
#include "entity.h"

namespace kglt {

const VertexData& Entity::shared_data() const {
    return scene_.newmesh(mesh_).shared_data();
}

newmesh::Mesh& Entity::_mesh_ref() {
    return scene_.newmesh(mesh_);
}

}
