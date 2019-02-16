//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <limits>

#include "mesh.h"
#include "adjacency_info.h"

#include "../window.h"
#include "../asset_manager.h"
#include "../loader.h"
#include "../material.h"
#include "../hardware_buffer.h"
#include "../renderers/renderer.h"
#include "private.h"

#include "../procedural/mesh.h"

namespace smlt {


Mesh::Mesh(MeshID id,
    AssetManager *resource_manager,
    VertexSpecification vertex_specification):
        Resource(resource_manager),
        generic::Identifiable<MeshID>(id) {

    reset(vertex_specification);

    vertex_data_->signal_update_complete().connect([this]() {
        // Mark the AABB as dirty so it will be rebuilt on next access
        aabb_dirty_ = true;
    });
}

void Mesh::reset(VertexSpecification vertex_specification) {
    adjacency_.reset();

    submeshes_.clear();
    ordered_submeshes_.clear();

    animation_type_ = MESH_ANIMATION_TYPE_NONE;
    animation_frames_ = 0;

    vertex_data_ = std::make_shared<VertexData>(vertex_specification);

    // When the vertex data updates, update the hardware buffer
    vertex_data->signal_update_complete().connect([this]() {
        shared_vertex_buffer_dirty_ = true;
    });
}

Mesh::~Mesh() {
    assert(ordered_submeshes_.size() == submeshes_.size());
    vertex_data_.reset();
}

void Mesh::each(std::function<void (const std::string &, SubMeshPtr)> func) const {
    return each_submesh(func);
}

void Mesh::each_submesh(std::function<void (const std::string&, SubMesh*)> func) const {
    assert(ordered_submeshes_.size() == submeshes_.size());

    // Respect insertion order while iterating
    // Copy array in case of deletions during iteration
    auto to_iterate = this->ordered_submeshes_;

    for(SubMesh* submesh: to_iterate) {
        auto sm = submesh->shared_from_this();
        if(sm) {
            func(sm->name(), sm.get());
        }
    }
}

void Mesh::clear() {
    //Delete the submeshes and clear the shared data
    submeshes_.clear();
    vertex_data->clear();
    rebuild_aabb();
}

void Mesh::enable_animation(MeshAnimationType animation_type, uint32_t animation_frames, MeshFrameDataPtr data) {
    if(animation_type_ != MESH_ANIMATION_TYPE_NONE) {
        throw std::logic_error("Tried to re-enable animations on an animated mesh");
    }

    if(!animation_frames) {
        throw std::logic_error("You must specify the number of frames when enabling mesh animations");
    }

    animation_type_ = animation_type;
    animation_frames_ = animation_frames;
    animated_frame_data_ = data;

    signal_animation_enabled_(this, animation_type_, animation_frames_);
}

void Mesh::rebuild_aabb() const {
    AABB& result = aabb_;

    if(!this->submesh_count()) {
        result = AABB();
        return;
    }

    float max = std::numeric_limits<float>::max();
    float min = std::numeric_limits<float>::min();

    result.set_min(smlt::Vec3(max, max, max));
    result.set_max(smlt::Vec3(min, min, min));

    each([&result](const std::string& name, SubMesh* mesh) {
        auto sm_min = mesh->aabb().min();
        auto sm_max = mesh->aabb().max();

        if(sm_min.x < result.min().x) result.set_min_x(sm_min.x);
        if(sm_min.y < result.min().y) result.set_min_y(sm_min.y);
        if(sm_min.z < result.min().z) result.set_min_z(sm_min.z);

        if(sm_max.x > result.max().x) result.set_max_x(sm_max.x);
        if(sm_max.y > result.max().y) result.set_max_y(sm_max.y);
        if(sm_max.z > result.max().z) result.set_max_z(sm_max.z);
    });

    aabb_dirty_ = false;
}

const AABB &Mesh::aabb() const {
    if(aabb_dirty_) {
        rebuild_aabb();
    }

    return aabb_;
}

SubMesh* Mesh::new_submesh_with_material(
    const std::string& name,
    MaterialID material,
    MeshArrangement arrangement, IndexType index_type) {

    if(submeshes_.count(name)) {
        throw std::runtime_error("Attempted to create a duplicate submesh with name: " + name);
    }

    auto new_submesh = SubMesh::create(this, name, material, arrangement, index_type);
    submeshes_.insert(std::make_pair(name, new_submesh));
    ordered_submeshes_.push_back(new_submesh.get());
    signal_submesh_created_(id(), new_submesh.get());

    // Mark the AABB as dirty so it will be rebuilt on next access
    aabb_dirty_ = true;

    new_submesh->index_data_->signal_update_complete().connect([this]() {
        aabb_dirty_ = true;
    });

    return new_submesh.get();
}

SubMesh* Mesh::new_submesh(
    const std::string& name,
    MeshArrangement arrangement, IndexType index_type) {

    return new_submesh_with_material(
        name,
        resource_manager().clone_default_material(),        
        arrangement,
        index_type
    );
}

SubMeshPtr Mesh::new_submesh_as_icosphere(const std::string& name, MaterialID material, float diameter, uint32_t subdivisions) {
    SubMesh* sm = new_submesh_with_material(name, material, MESH_ARRANGEMENT_TRIANGLES);

    procedural::mesh::icosphere(sm, diameter, subdivisions);

    return sm;
}

SubMesh* Mesh::new_submesh_as_box(const std::string& name, MaterialID material, float width, float height, float depth, const Vec3& offset) {
    SubMesh* sm = new_submesh_with_material(
        name,
        material,
        MESH_ARRANGEMENT_TRIANGLES
    );

    auto vd = sm->vertex_data.get();
    auto id = sm->index_data.get();

    float ox = offset.x;
    float oy = offset.y;
    float oz = offset.z;

    float rx = width * 0.5f;
    float ry = height * 0.5f;
    float rz = depth * 0.5f;

    //front and back
    for(int32_t z: { -1, 1 }) {
        uint32_t count = vd->count();

        vd->position(ox + -1 * rx, oy + -1 * ry, oz + z * rz);
        vd->tex_coord0(0, 0);
        vd->tex_coord1(0, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(ox + 1 * rx, oy + -1 * ry, oz + z * rz);
        vd->tex_coord0(1, 0);
        vd->tex_coord1(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(ox + 1 * rx,  oy + 1 * ry, oz + z * rz);
        vd->tex_coord0(1, 1);
        vd->tex_coord1(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(ox + -1 * rx, oy + 1 * ry, oz + z * rz);
        vd->tex_coord0(0, 1);
        vd->tex_coord1(0, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        if(z > 0) {
            id->index(count);
            id->index(count + 1);
            id->index(count + 2);

            id->index(count);
            id->index(count + 2);
            id->index(count + 3);
        } else {
            id->index(count);
            id->index(count + 2);
            id->index(count + 1);

            id->index(count);
            id->index(count + 3);
            id->index(count + 2);
        }
    }

    //left and right
    for(int32_t x: { -1, 1 }) {
        uint32_t count = vd->count();

        vd->position(ox + x * rx, oy + -1 * ry, oz + -1 * rz);
        vd->tex_coord0(0, 0);
        vd->tex_coord1(0, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(ox + x * rx, oy + 1 * ry, oz + -1 * rz);
        vd->tex_coord0(1, 0);
        vd->tex_coord1(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(ox + x * rx, oy + 1 * ry, oz + 1 * rz);
        vd->tex_coord0(1, 1);
        vd->tex_coord1(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(ox + x * rx, oy + -1 * ry, oz + 1 * rz);
        vd->tex_coord0(0, 1);
        vd->tex_coord1(0, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        if(x > 0) {
            id->index(count);
            id->index(count + 1);
            id->index(count + 2);

            id->index(count);
            id->index(count + 2);
            id->index(count + 3);
        } else {
            id->index(count);
            id->index(count + 2);
            id->index(count + 1);

            id->index(count);
            id->index(count + 3);
            id->index(count + 2);
        }

    }

    //top and bottom
    for(int32_t y: { -1, 1 }) {
        uint32_t count = vd->count();

        vd->position(ox + 1 * rx, oy + y * ry, oz + -1 * rz);
        vd->tex_coord0(0, 0);
        vd->tex_coord1(0, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position(ox + -1 * rx,  oy + y * ry, oz + -1 * rz);
        vd->tex_coord0(1, 0);
        vd->tex_coord1(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position(ox + -1 * rx, oy + y * ry, oz + 1 * rz);
        vd->tex_coord0(1, 1);
        vd->tex_coord1(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position(ox + 1 * rx, oy + y * ry, oz + 1 * rz);
        vd->tex_coord0(0, 1);
        vd->tex_coord1(0, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        if(y > 0) {
            id->index(count);
            id->index(count + 1);
            id->index(count + 2);

            id->index(count);
            id->index(count + 2);
            id->index(count + 3);
        } else {
            id->index(count);
            id->index(count + 2);
            id->index(count + 1);

            id->index(count);
            id->index(count + 3);
            id->index(count + 2);
        }

    }

    vd->done();
    id->done();

    return sm;
}

SubMesh* Mesh::new_submesh_as_rectangle(const std::string& name, MaterialID material, float width, float height, const smlt::Vec3& offset) {
    SubMesh* sm = new_submesh_with_material(
        name,
        material,
        MESH_ARRANGEMENT_TRIANGLES
    );

    float x_offset = offset.x;
    float y_offset = offset.y;
    float z_offset = offset.z;

    auto idx_offset = sm->vertex_data->count();

    auto spec = sm->vertex_data->specification();
    sm->vertex_data->move_to_end();

    //Build some shared vertex data
    sm->vertex_data->position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(0.0, 0.0);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(0.0, 0.0);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->move_next();

    sm->vertex_data->position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(1.0, 0.0);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(1.0, 0.0);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->move_next();

    sm->vertex_data->position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(1.0, 1.0);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(1.0, 1.0);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->move_next();

    sm->vertex_data->position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(0.0, 1.0);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(0.0, 1.0);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->done();

    sm->index_data->index(idx_offset + 0);
    sm->index_data->index(idx_offset + 1);
    sm->index_data->index(idx_offset + 2);

    sm->index_data->index(idx_offset + 0);
    sm->index_data->index(idx_offset + 2);
    sm->index_data->index(idx_offset + 3);
    sm->index_data->done();

    return sm;
}

void Mesh::delete_submesh(const std::string& name) {
    auto it = submeshes_.find(name);
    if(it != submeshes_.end()) {
        auto submesh = (*it).second;
        submeshes_.erase(it);
        ordered_submeshes_.remove(submesh.get());
        signal_submesh_destroyed_(id(), submesh.get());
        aabb_dirty_ = true;
    }
}

SubMesh* Mesh::first_submesh() const {
    if(submeshes_.empty()) {
        return nullptr;
    }

    return ordered_submeshes_.front();
}

void Mesh::set_material_id(MaterialID material) {
    each([=](const std::string& name, SubMesh* mesh) {
        mesh->set_material_id(material);
    });
}

void Mesh::transform_vertices(const smlt::Mat4& transform) {
    vertex_data->move_to_start();

    for(uint32_t i = 0; i < vertex_data->count(); ++i) {
        if(vertex_data->specification().has_positions()) {
            smlt::Vec3 v = vertex_data->position_at<Vec3>(i);
            v = v.transformed_by(transform);
            vertex_data->position(v);
        }

        if(vertex_data->specification().has_normals()) {
            smlt::Vec3 n;
            vertex_data->normal_at(i, n);
            n = n.rotated_by(transform);
            vertex_data->normal(n.normalized());
        }
        vertex_data->move_next();
    }
    vertex_data->done();
}

void Mesh::set_diffuse(const smlt::Colour& colour) {
    vertex_data->move_to_start();
    for(uint32_t i = 0; i < vertex_data->count(); ++i) {
        vertex_data->diffuse(colour);
        vertex_data->move_next();
    }
    vertex_data->done();
}

void Mesh::normalize() {
    float dia = this->diameter();
    float scaling = 1.0 / dia;

    smlt::Mat4 scale_matrix = Mat4::as_scaling(scaling);

    transform_vertices(scale_matrix);
}

void Mesh::reverse_winding() {
    each([=](const std::string& name, SubMesh* mesh) {
        mesh->reverse_winding();
    });
}

SubMesh* Mesh::submesh(const std::string& name) {
    auto it = submeshes_.find(name);
    if(it != submeshes_.end()) {
        return it->second.get();
    }

    return nullptr;
}

void Mesh::prepare_buffers(Renderer* renderer) {
    if(shared_vertex_buffer_dirty_ || !shared_vertex_buffer_) {
        sync_buffer<VertexData, Renderer>(
            &shared_vertex_buffer_, vertex_data_.get(),
            renderer,
            HARDWARE_BUFFER_VERTEX_ATTRIBUTES
        );
        shared_vertex_buffer_dirty_ = false;
    }
}

void Mesh::generate_adjacency_info() {
    adjacency_.reset(new AdjacencyInfo(this));
    adjacency_->rebuild();
}


}
