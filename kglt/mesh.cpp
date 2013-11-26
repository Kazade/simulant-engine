#include <limits>

#include "kazbase/unicode.h"
#include "window_base.h"
#include "resource_manager.h"
#include "mesh.h"
#include "loader.h"
#include "scene.h"
#include "material.h"

namespace kglt {

Mesh::Mesh(ResourceManager *resource_manager, MeshID id):
    Resource(resource_manager),
    generic::Identifiable<MeshID>(id),
    shared_data_(resource_manager->scene()),
    normal_debug_mesh_(0) {

}

void Mesh::clear() {
    //Delete the submeshes and clear the shared data
    for(SubMeshIndex i: submesh_ids()) {
        delete_submesh(i);
    }
    submeshes_.clear();
    shared_data().clear();
}

Scene& Mesh::scene() {
    return resource_manager().scene();
}

void Mesh::enable_debug(bool value) {
    if(value) {
        //This maintains a lock on the material
        MaterialID mid = resource_manager().new_material();

        resource_manager().window().loader_for(
            "kglt/materials/diffuse_render.kglm"
        )->into(resource_manager().material(mid));

        normal_debug_mesh_ = new_submesh(mid, MESH_ARRANGEMENT_LINES, false);

        //Go through the submeshes, and for each index draw a normal line
        for(SubMeshIndex smi: submesh_ids()) {
            for(uint16_t idx: submesh(smi).index_data().all()) {
                kmVec3 pos1 = submesh(smi).vertex_data().position_at(idx);
                kmVec3 n = submesh(smi).vertex_data().normal_at(idx);
                kmVec3Scale(&n, &n, 10.0);

                kmVec3 pos2;
                kmVec3Add(&pos2, &pos1, &n);

                submesh(normal_debug_mesh_).vertex_data().position(pos1);
                submesh(normal_debug_mesh_).vertex_data().diffuse(kglt::Colour::RED);
                int16_t next_index = submesh(normal_debug_mesh_).vertex_data().move_next();
                submesh(normal_debug_mesh_).index_data().index(next_index - 1);

                submesh(normal_debug_mesh_).vertex_data().position(pos2);
                submesh(normal_debug_mesh_).vertex_data().diffuse(kglt::Colour::RED);
                next_index = submesh(normal_debug_mesh_).vertex_data().move_next();
                submesh(normal_debug_mesh_).index_data().index(next_index - 1);
            }
        }
        submesh(normal_debug_mesh_).vertex_data().done();
        submesh(normal_debug_mesh_).index_data().done();
    } else {
        if(normal_debug_mesh_) {
            delete_submesh(normal_debug_mesh_);
            normal_debug_mesh_ = 0;
        }
    }
}

SubMeshIndex Mesh::new_submesh(    
    MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices) {

    static SubMeshIndex counter = 0;

    SubMeshIndex idx = ++counter;

    submeshes_.push_back(SubMesh::create(*this, material, arrangement, uses_shared_vertices));
    submeshes_by_index_[idx] = submeshes_[submeshes_.size()-1];

    signal_submeshes_changed_.emit();

    return idx;
}

void Mesh::delete_submesh(SubMeshIndex index) {
    if(!container::contains(submeshes_by_index_, index)) {
        throw std::out_of_range("Tried to delete a submesh that doesn't exist");
    }

    submeshes_.erase(std::remove(submeshes_.begin(), submeshes_.end(), submeshes_by_index_[index]), submeshes_.end());
    submeshes_by_index_.erase(index);

    signal_submeshes_changed_.emit();
}

void Mesh::set_material_id(MaterialID material) {
    for(SubMesh::ptr sm: submeshes_) {
        sm->set_material_id(material);
    }
}

void Mesh::reverse_winding() {
    for(SubMesh::ptr sm: submeshes_) {
        sm->reverse_winding();
    }
}

void Mesh::set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass) {
    for(SubMesh::ptr sm: submeshes_) {
        sm->set_texture_on_material(unit, tex, pass);
    }
}

SubMesh& Mesh::submesh(SubMeshIndex index) {
    assert(index > 0);
    return *submeshes_by_index_[index];
}

SubMesh::SubMesh(
    Mesh& parent, MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices):
    parent_(parent),
    arrangement_(arrangement),
    uses_shared_data_(uses_shared_vertices),
    vertex_data_(parent.scene()),
    index_data_(parent.scene()) {

    if(!material) {
        //Set the material to the default one (store the pointer to increment the ref-count)
        material_ = parent_.resource_manager().material(parent_.resource_manager().scene().default_material_id()).__object;
    } else {
        set_material_id(material);
    }

    //The vertex data might already have been set up, so mark it as
    //dirty if we are using shared data.
    this->vertex_data_dirty_ = uses_shared_data_ && !vertex_data().empty();

    vrecalc_ = vertex_data().signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));
    irecalc_ = index_data().signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));

    vertex_data().signal_update_complete().connect([&]{
        this->vertex_data_dirty_ = true;
    });

    index_data().signal_update_complete().connect([&]{
        this->index_data_dirty_ = true;
    });
}

void SubMesh::_bind_vertex_array_object() {
    vertex_array_object_.bind();

    vertex_array_object_.vertex_buffer_bind();
    vertex_array_object_.index_buffer_bind();
}

void SubMesh::_update_vertex_array_object() {
    if(vertex_data_dirty_) {
        vertex_array_object_.vertex_buffer_update(vertex_data().count() * sizeof(Vertex), vertex_data()._raw_data());
        vertex_data_dirty_ = false;
    }

    if(index_data_dirty_) {
        vertex_array_object_.index_buffer_update(index_data().count() * sizeof(uint16_t), index_data()._raw_data());
        index_data_dirty_ = false;

        if(vertex_data().empty()) {
            L_WARN("Uploading index data to GL without any vertices");
        }
    }
}

const MaterialID SubMesh::material_id() const {
    return material_->id();
}

void SubMesh::set_material_id(MaterialID mat) {
    //Set the material, store the shared_ptr to increment the ref count
    material_ = parent_.resource_manager().material(mat).__object;
}

void SubMesh::transform_vertices(const kmMat4& transformation) {
    vertex_data().move_to_start();
    for(uint16_t i = 0; i < vertex_data().count(); ++i) {
        kmVec3 v = vertex_data().position_at(i);

        kmVec3MultiplyMat4(&v, &v, &transformation);

        vertex_data().position(v);
        vertex_data().move_next();
    }
    vertex_data().done();
}

void SubMesh::reverse_winding() {
    if(arrangement_ != MESH_ARRANGEMENT_TRIANGLES) {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    std::vector<uint16_t> original = index_data().all();

    index_data().clear();
    for(uint32_t i = 0; i < original.size() / 3; ++i) {
        index_data().index(original[i * 3]);
        index_data().index(original[(i * 3) + 2]);
        index_data().index(original[(i * 3) + 1]);
    }
    index_data().done();
}

/**
 * @brief SubMesh::recalc_bounds
 *
 * Recalculate the bounds of the submesh. This involves interating over all of the
 * vertices that make up the submesh and so is potentially quite slow. This happens automatically
 * when vertex_data().done() or index_data().done() are called.
 */
void SubMesh::_recalc_bounds() {
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

void SubMesh::set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass) {
    material_->technique().pass(pass).set_texture_unit(unit, tex);
}

SubMesh::~SubMesh() {
    vrecalc_.disconnect();
    irecalc_.disconnect();
}

}
