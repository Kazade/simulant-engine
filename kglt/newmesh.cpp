#include "newmesh.h"

namespace kglt {
namespace newmesh {

Mesh::Mesh(Scene* scene, newmesh::MeshID id):
    generic::Identifiable<newmesh::MeshID>(id),
    scene_(*scene) {

}

SubMeshIndex Mesh::new_submesh(
    MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices) {

    submeshes_.push_back(SubMesh::create(*this, material, arrangement, uses_shared_vertices));
    return submeshes_.size() - 1;
}

SubMesh& Mesh::submesh(uint16_t index) {
    return *submeshes_.at(index);
}

SubMesh::SubMesh(
    Mesh& parent, MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices):
    parent_(parent),
    material_(material),
    arrangement_(arrangement),
    uses_shared_data_(uses_shared_vertices) {

}

VertexData& SubMesh::vertex_data() {
    if(uses_shared_data_) { return parent_.shared_data(); }
    return vertex_data_;
}

IndexData& SubMesh::index_data() {
    return index_data_;
}

const VertexData& SubMesh::vertex_data() const {
    if(uses_shared_data_) { return parent_.shared_data(); }
    return vertex_data_;
}

const IndexData& SubMesh::index_data() const {
    return index_data_;
}

SubMesh::~SubMesh() {}

}
}
