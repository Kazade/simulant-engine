#include <limits>

#include "kazbase/unicode.h"
#include "window_base.h"
#include "resource_manager.h"
#include "mesh.h"
#include "loader.h"
#include "material.h"

namespace kglt {

Mesh::Mesh(ResourceManager *resource_manager, MeshID id):
    Resource(resource_manager),
    generic::Identifiable<MeshID>(id),
    normal_debug_mesh_(0) {

    //FIXME: Somehow we need to specify if the shared data is modified repeatedly etc.
    shared_data_buffer_object_ = BufferObject::create(BUFFER_OBJECT_VERTEX_DATA, MODIFY_ONCE_USED_FOR_RENDERING);

    shared_data().signal_update_complete().connect([&]{
        this->shared_data_dirty_ = true;
    });
}

void Mesh::clear() {
    //Delete the submeshes and clear the shared data
    for(SubMeshIndex i: submesh_ids()) {
        delete_submesh(i);
    }
    submeshes_.clear();
    shared_data().clear();
}

const AABB Mesh::aabb() const {
    //FIXME: This should totally be cached for speed
    AABB result;

    float max = std::numeric_limits<float>::max();
    float min = std::numeric_limits<float>::min();

    result.min = kglt::Vec3(max, max, max);
    result.max = kglt::Vec3(min, min, min);

    for(auto mesh: submeshes_) {
        if(mesh->aabb().min.x < result.min.x) result.min.x = mesh->aabb().min.x;
        if(mesh->aabb().min.y < result.min.y) result.min.y = mesh->aabb().min.y;
        if(mesh->aabb().min.z < result.min.z) result.min.z = mesh->aabb().min.z;

        if(mesh->aabb().max.x > result.max.x) result.max.x = mesh->aabb().max.x;
        if(mesh->aabb().max.y > result.max.y) result.max.y = mesh->aabb().max.y;
        if(mesh->aabb().max.z > result.max.z) result.max.z = mesh->aabb().max.z;
    }

    return result;
}

void Mesh::enable_debug(bool value) {
    if(value) {
        //This maintains a lock on the material
        MaterialID mid = resource_manager().new_material();

        resource_manager().window().loader_for(
            "material_loader",
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

    submeshes_.push_back(SubMesh::create(*this, idx, material, arrangement, uses_shared_vertices));
    submeshes_by_index_[idx] = submeshes_[submeshes_.size()-1];

    signal_submeshes_changed_();

    return idx;
}

SubMeshIndex Mesh::new_submesh_as_box(MaterialID material, float width, float height, float depth, const Vec3& offset) {
    SubMeshIndex ret = new_submesh(material, MESH_ARRANGEMENT_TRIANGLES, false);

    auto& sm = submesh(ret);
    auto& vd = sm.vertex_data();
    auto& id = sm.index_data();

    float x_offset = offset.x;
    float y_offset = offset.y;
    float z_offset = offset.z;

    float rx = width * 0.5f;
    float ry = height * 0.5f;
    float rz = depth * 0.5f;

    //front and back
    for(int32_t z: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = vd.count();

            vd.position(-1 * rx, -1 * ry, z * rz);
            vd.tex_coord0(0, 0);
            vd.tex_coord1(0, 0);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, 0, z);
            vd.move_next();

            vd.position( 1 * rx, -1 * ry, z * rz);
            vd.tex_coord0(1, 0);
            vd.tex_coord1(1, 0);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, 0, z);
            vd.move_next();

            vd.position( 1 * rx,  1 * ry, z * rz);
            vd.tex_coord0(1, 1);
            vd.tex_coord1(1, 1);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, 0, z);
            vd.move_next();

            vd.position(-1 * rx,  1 * ry, z * rz);
            vd.tex_coord0(0, 1);
            vd.tex_coord1(0, 1);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, 0, z);
            vd.move_next();

            if(z > 0) {
                id.index(count);
                id.index(count + 1);
                id.index(count + 2);

                id.index(count);
                id.index(count + 2);
                id.index(count + 3);
            } else {
                id.index(count);
                id.index(count + 2);
                id.index(count + 1);

                id.index(count);
                id.index(count + 3);
                id.index(count + 2);
            }
        }
    }

    //left and right
    for(int32_t x: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = vd.count();

            vd.position( x * rx, -1 * ry, -1 * rz);
            vd.tex_coord0(0, 0);
            vd.tex_coord1(0, 0);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(x, 0, 0);
            vd.move_next();

            vd.position( x * rx,  1 * ry, -1 * rz);
            vd.tex_coord0(1, 0);
            vd.tex_coord1(1, 0);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(x, 0, 0);
            vd.move_next();

            vd.position( x * rx,  1 * ry, 1 * rz);
            vd.tex_coord0(1, 1);
            vd.tex_coord1(1, 1);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(x, 0, 0);
            vd.move_next();

            vd.position(x * rx, -1 * ry, 1 * rz);
            vd.tex_coord0(0, 1);
            vd.tex_coord1(0, 1);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(x, 0, 0);
            vd.move_next();

            if(x > 0) {
                id.index(count);
                id.index(count + 1);
                id.index(count + 2);

                id.index(count);
                id.index(count + 2);
                id.index(count + 3);
            } else {
                id.index(count);
                id.index(count + 2);
                id.index(count + 1);

                id.index(count);
                id.index(count + 3);
                id.index(count + 2);
            }
        }
    }

    //top and bottom
    for(int32_t y: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = vd.count();

            vd.position( 1 * rx, y * ry, -1 * rz);
            vd.tex_coord0(0, 0);
            vd.tex_coord1(0, 0);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, y, 0);
            vd.move_next();

            vd.position( -1 * rx,  y * ry, -1 * rz);
            vd.tex_coord0(1, 0);
            vd.tex_coord1(1, 0);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, y, 0);
            vd.move_next();

            vd.position( -1 * rx,  y * ry, 1 * rz);
            vd.tex_coord0(1, 1);
            vd.tex_coord1(1, 1);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, y, 0);
            vd.move_next();

            vd.position( 1 * rx, y * ry, 1 * rz);
            vd.tex_coord0(0, 1);
            vd.tex_coord1(0, 1);
            vd.diffuse(kglt::Colour::WHITE);
            vd.normal(0, y, 0);
            vd.move_next();

            if(y > 0) {
                id.index(count);
                id.index(count + 1);
                id.index(count + 2);

                id.index(count);
                id.index(count + 2);
                id.index(count + 3);
            } else {
                id.index(count);
                id.index(count + 2);
                id.index(count + 1);

                id.index(count);
                id.index(count + 3);
                id.index(count + 2);
            }
        }
    }

    // Apply offset
    vd.move_to_start();
    for(uint16_t i = 0; i < vd.count(); ++i) {
        vd.position(vd.position_at(i) + kglt::Vec3(x_offset, y_offset, z_offset));
        vd.move_next();
    }

    vd.done();
    id.done();

    return ret;
}

SubMeshIndex Mesh::new_submesh_as_rectangle(MaterialID material, float width, float height, const kglt::Vec3& offset) {
    SubMeshIndex ret = new_submesh(material, MESH_ARRANGEMENT_TRIANGLES, false);

    auto& sm = submesh(ret);

    float x_offset = offset.x;
    float y_offset = offset.y;
    float z_offset = offset.z;

    //Build some shared vertex data
    sm.vertex_data().position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    sm.vertex_data().diffuse(kglt::Colour::WHITE);
    sm.vertex_data().tex_coord0(0.0, 0.0);
    sm.vertex_data().tex_coord1(0.0, 0.0);
    sm.vertex_data().tex_coord2(0.0, 0.0);
    sm.vertex_data().tex_coord3(0.0, 0.0);
    sm.vertex_data().normal(0, 0, 1);
    sm.vertex_data().move_next();

    sm.vertex_data().position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    sm.vertex_data().diffuse(kglt::Colour::WHITE);
    sm.vertex_data().tex_coord0(1.0, 0.0);
    sm.vertex_data().tex_coord1(1.0, 0.0);
    sm.vertex_data().tex_coord2(1.0, 0.0);
    sm.vertex_data().tex_coord3(1.0, 0.0);
    sm.vertex_data().normal(0, 0, 1);
    sm.vertex_data().move_next();

    sm.vertex_data().position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    sm.vertex_data().diffuse(kglt::Colour::WHITE);
    sm.vertex_data().tex_coord0(1.0, 1.0);
    sm.vertex_data().tex_coord1(1.0, 1.0);
    sm.vertex_data().tex_coord2(1.0, 1.0);
    sm.vertex_data().tex_coord3(1.0, 1.0);
    sm.vertex_data().normal(0, 0, 1);
    sm.vertex_data().move_next();

    sm.vertex_data().position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    sm.vertex_data().diffuse(kglt::Colour::WHITE);
    sm.vertex_data().tex_coord0(0.0, 1.0);
    sm.vertex_data().tex_coord1(0.0, 1.0);
    sm.vertex_data().tex_coord2(0.0, 1.0);
    sm.vertex_data().tex_coord3(0.0, 1.0);
    sm.vertex_data().normal(0, 0, 1);
    sm.vertex_data().move_next();
    sm.vertex_data().done();

    sm.index_data().index(0);
    sm.index_data().index(1);
    sm.index_data().index(2);

    sm.index_data().index(0);
    sm.index_data().index(2);
    sm.index_data().index(3);
    sm.index_data().done();

    return ret;
}

void Mesh::delete_submesh(SubMeshIndex index) {
    if(!container::contains(submeshes_by_index_, index)) {
        throw std::out_of_range("Tried to delete a submesh that doesn't exist");
    }

    submeshes_.erase(std::remove(submeshes_.begin(), submeshes_.end(), submeshes_by_index_[index]), submeshes_.end());
    submeshes_by_index_.erase(index);

    signal_submeshes_changed_();
}

void Mesh::set_material_id(MaterialID material) {
    for(SubMesh::ptr sm: submeshes_) {
        sm->set_material_id(material);
    }
}

void Mesh::transform_vertices(const kglt::Mat4& transform, bool include_submeshes) {
    shared_data().move_to_start();

    for(int i = 0; i < shared_data().count(); ++i) {
        kglt::Vec3 v = shared_data().position_at(i);
        kmVec3MultiplyMat4(&v, &v, &transform);
        shared_data().position(v);

        kglt::Vec3 n = shared_data().normal_at(i);
        kmVec3MultiplyMat4(&n, &n, &transform);
        shared_data().normal(n.normalized());

        shared_data().move_next();
    }
    shared_data().done();

    if(include_submeshes) {
        for(auto mesh: submeshes_) {
            if(!mesh->uses_shared_vertices()) {
                mesh->transform_vertices(transform);
            }
        }
    }
}

void Mesh::set_diffuse(const kglt::Colour& colour, bool include_submeshes) {
    shared_data().move_to_start();
    for(int i = 0; i < shared_data().count(); ++i) {
        shared_data().diffuse(colour);
        shared_data().move_next();
    }
    shared_data().done();

    if(include_submeshes) {
        for(auto mesh: submeshes_) {
            if(!mesh->uses_shared_vertices()) {
                mesh->set_diffuse(colour);
            }
        }
    }
}

void Mesh::normalize() {
    float dia = this->diameter();
    float scaling = 1.0 / dia;

    kglt::Mat4 scale_matrix;
    kmMat4Scaling(&scale_matrix, scaling, scaling, scaling);

    transform_vertices(scale_matrix);
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

void Mesh::_update_buffer_object() {
    if(shared_data_dirty_) {
        shared_data_buffer_object_->build(shared_data().count() * sizeof(Vertex), shared_data()._raw_data());
        shared_data_dirty_ = false;
    }
}

SubMesh& Mesh::submesh(SubMeshIndex index) {
    assert(index > 0);
    return *submeshes_by_index_[index];
}

SubMesh::SubMesh(
    Mesh& parent, SubMeshIndex idx, MaterialID material, MeshArrangement arrangement, bool uses_shared_vertices):
    parent_(parent),
    id_(idx),
    arrangement_(arrangement),
    uses_shared_data_(uses_shared_vertices) {

    /*
     * If we use shared vertices then we must reuse the buffer object from the mesh,
     * otherwise the VAO creates its own VBO
     */
    if(uses_shared_data_) {
        vertex_array_object_ = VertexArrayObject::create(parent_.shared_data_buffer_object_);
    } else {
        vertex_array_object_ = VertexArrayObject::create();
    }

    if(!material) {
        //Set the material to the default one (store the pointer to increment the ref-count)
        material_ = parent_.resource_manager().material(parent_.resource_manager().default_material_id()).__object;
    } else {
        set_material_id(material);
    }

    vrecalc_ = vertex_data().signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));
    irecalc_ = index_data().signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));

    if(!uses_shared_data_) {
        vertex_data().signal_update_complete().connect([&]{
            this->vertex_data_dirty_ = true;
        });
    }

    index_data().signal_update_complete().connect([&]{
        this->index_data_dirty_ = true;
    });
}

void SubMesh::_bind_vertex_array_object() {
    vertex_array_object_->bind();
}

void SubMesh::_update_vertex_array_object() {
    if(uses_shared_vertices()) {
        parent_._update_buffer_object();
    } else if(vertex_data_dirty_) {
        vertex_array_object_->vertex_buffer_update(vertex_data().count() * sizeof(Vertex), vertex_data()._raw_data());
        vertex_data_dirty_ = false;
    }

    if(index_data_dirty_) {
        vertex_array_object_->index_buffer_update(index_data().count() * sizeof(uint16_t), index_data()._raw_data());
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

void SubMesh::transform_vertices(const kglt::Mat4& transformation) {
    if(uses_shared_data_) {
        throw LogicError("Tried to transform shared_data, use Mesh::transform_vertices instead");
    }

    vertex_data().move_to_start();
    for(uint16_t i = 0; i < vertex_data().count(); ++i) {
        kglt::Vec3 v = vertex_data().position_at(i);

        kmVec3MultiplyMat4(&v, &v, &transformation);

        vertex_data().position(v);

        if(vertex_data().has_normals()) {
            kglt::Vec3 n = vertex_data().normal_at(i);
            kmVec3MultiplyMat4(&n, &n, &transformation);
            vertex_data().normal(n.normalized());
        }

        vertex_data().move_next();
    }
    vertex_data().done();
}

void SubMesh::set_diffuse(const kglt::Colour& colour) {
    if(uses_shared_data_) {
        throw LogicError("Tried to set the diffuse colour on shared_data, use Mesh::set_diffuse instead");
    }

    vertex_data().move_to_start();
    for(uint16_t i = 0; i < vertex_data().count(); ++i) {
        vertex_data().diffuse(colour);
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
        kmAABB3Initialize(&bounds_, nullptr, 0, 0, 0);
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
    material_->pass(pass).set_texture_unit(unit, tex);
}

/**
 * @brief SubMesh::generate_texture_coordinates_cube
 * @param texture - which texture unit to generate coordinates for
 *
 * Generates cube coordinates.
 *
 * WARNING: Will not duplicate vertices. If you share vertices across polygons this
 * will currently not give correct results. In future a "duplicate_vertices" argument
 * might be added.
 */
void SubMesh::generate_texture_coordinates_cube(uint32_t texture) {
    auto& vd = vertex_data();

    float box_size = std::max(bounds_.width(), std::max(bounds_.height(), bounds_.depth()));

    vd.move_to_start();
    for(uint16_t i = 0; i < vd.count(); ++i) {
        auto v = vd.normal_at(i); // Get the vertex normal

        // Work out the component with the largest value
        float absx = fabs(v.x);
        float absy = fabs(v.y);
        float absz = fabs(v.z);

        bool x = (absx > absy && absx > absz);
        bool y = (absy > absx && absy > absz);

        // Generate a orthagonal direction vector

        kglt::Vec3 dir(0, 0, 0);

        if(x) {
            dir.x = (v.x < 0) ? -1 : 1;
        } else if(y) {
            dir.y = (v.y < 0) ? -1 : 1;
        } else {
            dir.z = (v.z < 0) ? -1 : 1;
        }

        // Create a plane at the origin with the opposite direction
        kmPlane plane;
        kmPlaneFill(&plane, -dir.x, -dir.y, -dir.z, 0);

        kglt::Vec3 v1 = vd.position_at(i) - bounds_.min;
        kglt::Vec3 v2 = v1 + dir;

        // Project the vertex position onto the plane
        kglt::Vec3 final;
        kmPlaneIntersectLine(&final, &plane, &v1, &v2);

        //Scale the final position on the plane by the size of the box
        // and subtract the lower corner so that everything is relative to 0,0,0
        // and scaled between 0 and 1
        final /= box_size;

        // Finally, offset the uv coordinate to the right 'square' of the cubic texture
        if(x) {
            if(v.x >= 0) {
                final.x = 2.0 / 3.0 + (final.x / 3.0);
                final.y = 2.0 / 4.0 + (final.y / 4.0);
            } else {
                final.x = final.x / 3.0;
                final.y = 2.0 / 4.0 + (final.y / 4.0);
            }
        } else if(y) {
            if(v.y >= 0) {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = 3.0 / 4.0 + (final.y / 4.0);
            } else {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = 1.0 / 4.0 + (final.y / 4.0);
            }
        } else {
            if(v.z >= 0) {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = 2.0 / 4.0 + (final.y / 4.0);
            } else {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = (final.y / 4.0);
            }
        }

        switch(texture) {
            case 0: vd.tex_coord0(final.x, final.y);
            case 1: vd.tex_coord1(final.x, final.y);
            case 2: vd.tex_coord2(final.x, final.y);
            case 3: vd.tex_coord3(final.x, final.y);
        }
        vd.move_next();
    }

    vd.done();

}

SubMesh::~SubMesh() {
    vrecalc_.disconnect();
    irecalc_.disconnect();
}

}
