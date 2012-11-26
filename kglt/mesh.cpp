#include <limits>

#include "mesh.h"

namespace kglt {

Mesh::Mesh(Scene* scene, MeshID id):
    generic::Identifiable<MeshID>(id),
    scene_(*scene) {

}

void Mesh::clear() {
    //Delete the submeshes and clear the shared data
    for(SubMeshIndex i: submesh_ids()) {
        delete_submesh(i);
    }
    submeshes_.clear();
    shared_data().clear();
}

SubMeshIndex Mesh::new_submesh(    
    MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices) {

    static SubMeshIndex counter = 0;

    SubMeshIndex idx = ++counter;

    submeshes_.push_back(SubMesh::create(*this, material, arrangement, uses_shared_vertices));
    submeshes_by_index_[idx] = submeshes_[submeshes_.size()-1];

    return idx;
}

void Mesh::delete_submesh(SubMeshIndex index) {
    if(!container::contains(submeshes_by_index_, index)) {
        throw std::out_of_range("Tried to delete a submesh that doesn't exist");
    }

    submeshes_.erase(std::remove(submeshes_.begin(), submeshes_.end(), submeshes_by_index_[index]), submeshes_.end());
    submeshes_by_index_.erase(index);
}

SubMesh& Mesh::submesh(SubMeshIndex index) {
    assert(index > 0);
    return *submeshes_by_index_[index];
}

SubMesh::SubMesh(
    Mesh& parent, MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices):
    parent_(parent),
    material_(material),
    arrangement_(arrangement),
    uses_shared_data_(uses_shared_vertices) {

    vrecalc_ = vertex_data().signal_update_complete().connect(sigc::mem_fun(this, &SubMesh::recalc_bounds));
    irecalc_ = index_data().signal_update_complete().connect(sigc::mem_fun(this, &SubMesh::recalc_bounds));
}

/**
 * @brief SubMesh::recalc_bounds
 *
 * Recalculate the bounds of the submesh. This involves interating over all of the
 * vertices that make up the submesh and so is potentially quite slow. This happens automatically
 * when vertex_data().done() or index_data().done() are called.
 */
void SubMesh::recalc_bounds() {
    //Set the min bounds to the max
    kmVec3Fill(&bounds_.min, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    //Set the max bounds to the min
    kmVec3Fill(&bounds_.max, -1000000, -1000000, -1000000);

    if(!index_data().count()) {
        kmAABBInitialize(&bounds_, nullptr, 0, 0, 0);
        return;
    }

    for(uint16_t idx: index_data().all()) {
        kmVec3 pos = vertex_data().position_at(idx);
        if(pos.x < bounds_.min.x) bounds_.min.x = pos.x;
        if(pos.y < bounds_.min.y) bounds_.min.y = pos.y;
        if(pos.z < bounds_.min.z) bounds_.min.z = pos.z;

        if(pos.x > bounds_.max.x) bounds_.max.x = pos.x;
        if(pos.y > bounds_.max.y) bounds_.max.y = pos.y;
        if(pos.z > bounds_.max.z) bounds_.max.z = pos.z;
    }
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

SubMesh::~SubMesh() {
    vrecalc_.disconnect();
    irecalc_.disconnect();
}

}
