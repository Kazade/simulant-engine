//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <limits>

#include "mesh.h"
#include "adjacency_info.h"
#include "../assets/meshes/skeleton.h"

#include "../window.h"
#include "../asset_manager.h"
#include "../loader.h"
#include "../material.h"
#include "../renderers/renderer.h"
#include "private.h"

#include "../procedural/mesh.h"

namespace smlt {


Mesh::Mesh(MeshID id,
    AssetManager *asset_manager,
    VertexSpecification vertex_specification):
        Asset(asset_manager),
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

    animation_type_ = MESH_ANIMATION_TYPE_NONE;
    animation_frames_ = 0;

    vertex_data_ = std::make_shared<VertexData>(vertex_specification);

    delete skeleton_;
    skeleton_ = nullptr;
}

bool Mesh::add_skeleton(uint32_t num_joints) {
    if(!skeleton_) {
        skeleton_ = new Skeleton(this, num_joints);
        signal_skeleton_added_(skeleton_);
        return true;
    } else {
        return false;
    }
}

bool Mesh::has_skeleton() const {
    return skeleton_ != nullptr;
}

Mesh::~Mesh() {
    submeshes_.clear();
    vertex_data_.reset();

    delete skeleton_;
}

void Mesh::clear() {
    //Delete the submeshes and clear the shared data
    submeshes_.clear();
    vertex_data->clear();
    rebuild_aabb();
}

void Mesh::enable_animation(MeshAnimationType animation_type, uint32_t animation_frames, FrameUnpackerPtr data) {
    if(animation_type_ != MESH_ANIMATION_TYPE_NONE) {
        throw std::logic_error("Tried to re-enable animations on an animated mesh");
    }

    if(!animation_frames) {
        throw std::logic_error("You must specify the number of frames when enabling mesh animations");
    }

    if(animation_type == MESH_ANIMATION_TYPE_SKELETAL && !skeleton.get()) {
        throw std::logic_error("enable_animation called without skeleton");
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
    float min = std::numeric_limits<float>::lowest();

    result.set_min(smlt::Vec3(max, max, max));
    result.set_max(smlt::Vec3(min, min, min));

    for(auto mesh: submeshes_) {
        auto sm_min = mesh->aabb().min();
        auto sm_max = mesh->aabb().max();

        if(sm_min.x < result.min().x) result.set_min_x(sm_min.x);
        if(sm_min.y < result.min().y) result.set_min_y(sm_min.y);
        if(sm_min.z < result.min().z) result.set_min_z(sm_min.z);

        if(sm_max.x > result.max().x) result.set_max_x(sm_max.x);
        if(sm_max.y > result.max().y) result.set_max_y(sm_max.y);
        if(sm_max.z > result.max().z) result.set_max_z(sm_max.z);
    }

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

    if(has_submesh(name)) {
        throw std::runtime_error("Attempted to create a duplicate submesh with name: " + name);
    }

    auto mat = asset_manager().material(material);
    assert(mat);

    auto new_submesh = SubMesh::create(this, name, mat, arrangement, index_type);
    submeshes_.push_back(new_submesh);

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
        asset_manager().clone_default_material(),
        arrangement,
        index_type
    );
}

SubMeshPtr Mesh::new_submesh_as_sphere(const std::string& name,
    MaterialID material, float diameter, std::size_t slices, std::size_t stacks) {

    SubMesh* sm = new_submesh_with_material(name, material, MESH_ARRANGEMENT_TRIANGLES);

    procedural::mesh::sphere(sm, diameter, slices, stacks);

    return sm;
}

SubMeshPtr Mesh::new_submesh_as_capsule(
    const std::string& name,
    MaterialID material, float diameter, float length,
    std::size_t segment_count, std::size_t vertical_segment_count, std::size_t ring_count) {

    SubMesh* submesh = new_submesh_with_material(name, material, MESH_ARRANGEMENT_TRIANGLES);

    float radius = diameter * 0.5f;

    auto& vdata = submesh->vertex_data;
    auto& idata = submesh->index_data;

    float delta_ring_angle = ((PI / 2.0f) / ring_count);
    float delta_seg_angle = ((PI * 2.0f) / segment_count);

    float sphere_ratio = radius / (2 * radius + length);
    float cylinder_ratio = length / (2 * radius + length);
    float cylinder_length = length - diameter;

    auto offset = vertex_data->count();

    for(uint32_t ring = 0; ring <= ring_count; ++ring) {
        float r0 = radius * std::sin(ring * delta_ring_angle);
        float y0 = radius * std::cos(ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for(uint32_t seg = 0; seg <= segment_count; ++seg) {
            float x0 = r0 * std::cos(seg * delta_seg_angle);
            float z0 = r0 * std::sin(seg * delta_seg_angle);

            smlt::Vec3 new_point(x0, 0.5f * cylinder_length + y0, z0);

            smlt::Vec3 new_normal(x0, y0, z0);
            new_normal.normalize();

            smlt::Vec2 new_tex(
                (float) seg / (float) segment_count,
                (float) ring / (float) ring_count * sphere_ratio
            );

            // Add one vertex to the strip which makes up the sphere
            vdata->position(new_point);
            vdata->tex_coord0(new_tex);
            vdata->tex_coord1(new_tex);
            vdata->normal(new_normal);
            vdata->diffuse(smlt::Colour::WHITE);
            vdata->move_next();

            // each vertex (except the last) has six indices pointing to it
            idata->index(offset + segment_count + 1);
            idata->index(offset + segment_count);
            idata->index(offset);
            idata->index(offset + segment_count + 1);
            idata->index(offset);
            idata->index(offset + 1);

            offset ++;
        } // end for seg
    }

    // Cylinder part

    float delta_angle = ((PI * 2.0f) / segment_count);
    float delta_height = cylinder_length / (float) vertical_segment_count;

    for(uint16_t i = 1; i < vertical_segment_count; i++) {
        for (uint16_t j = 0; j <= segment_count; j++) {
            float x0 = radius * std::cos(j * delta_angle);
            float z0 = radius * std::sin(j * delta_angle);

            Vec3 new_point(
                x0,
                0.5f * cylinder_length - i * delta_height,
                z0
            );

            Vec3 new_normal(x0, 0, z0);
            new_normal.normalize();

            Vec2 new_tex(
                j / (float)segment_count,
                i / (float)vertical_segment_count * cylinder_ratio + sphere_ratio
            );

            vdata->position(new_point);
            vdata->tex_coord0(new_tex);
            vdata->tex_coord1(new_tex);
            vdata->normal(new_normal);
            vdata->diffuse(smlt::Colour::WHITE);
            vdata->move_next();

            idata->index(offset + segment_count + 1);
            idata->index(offset + segment_count);
            idata->index(offset);
            idata->index(offset + segment_count + 1);
            idata->index(offset);
            idata->index(offset + 1);

            offset ++;
        }
    }

    // Generate the group of rings for the sphere
    for(uint32_t ring = 0; ring <= ring_count; ring++) {
        float r0 = radius * sinf((PI / 2.0f) + ring * delta_ring_angle);
        float y0 = radius * cosf((PI / 2.0f) + ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for(uint32_t seg = 0; seg <= segment_count; seg++) {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            Vec3 new_point(
                x0,
                -0.5f * cylinder_length + y0,
                z0
            );

            Vec3 new_normal(x0, y0, z0);
            new_normal.normalize();

            Vec2 new_tex(
               (float) seg / (float) segment_count,
               (float) ring / (float) ring_count * sphere_ratio + cylinder_ratio + sphere_ratio
            );

            vdata->position(new_point);
            vdata->tex_coord0(new_tex);
            vdata->tex_coord1(new_tex);
            vdata->normal(new_normal);
            vdata->diffuse(smlt::Colour::WHITE);
            vdata->move_next();

            if (ring != ring_count) {
                // each vertex (except the last) has six indices pointing to it
                idata->index(offset + segment_count + 1);
                idata->index(offset + segment_count);
                idata->index(offset);
                idata->index(offset + segment_count + 1);
                idata->index(offset);
                idata->index(offset + 1);
            }
            offset++;
        } // end for seg
    } // end for ring

    idata->done();
    vdata->done();

    return submesh;
}

SubMeshPtr Mesh::new_submesh_as_icosphere(const std::string& name, MaterialID material, float diameter, uint32_t subdivisions) {
    SubMeshPtr sm = new_submesh_with_material(name, material, MESH_ARRANGEMENT_TRIANGLES);

    procedural::mesh::icosphere(sm, diameter, subdivisions);

    return sm;
}

SubMeshPtr Mesh::new_submesh_as_cube(const std::string& name, MaterialID material, float size) {
    return new_submesh_as_box(name, material, size, size, size);
}

bool Mesh::has_submesh(const std::string& name) const {
    return bool(find_submesh(name));
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
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(ox + 1 * rx, oy + -1 * ry, oz + z * rz);
        vd->tex_coord0(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(ox + 1 * rx,  oy + 1 * ry, oz + z * rz);
        vd->tex_coord0(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(ox + -1 * rx, oy + 1 * ry, oz + z * rz);
        vd->tex_coord0(0, 1);
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
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(ox + x * rx, oy + 1 * ry, oz + -1 * rz);
        vd->tex_coord0(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(ox + x * rx, oy + 1 * ry, oz + 1 * rz);
        vd->tex_coord0(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(ox + x * rx, oy + -1 * ry, oz + 1 * rz);
        vd->tex_coord0(0, 1);
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
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position(ox + -1 * rx,  oy + y * ry, oz + -1 * rz);
        vd->tex_coord0(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position(ox + -1 * rx, oy + y * ry, oz + 1 * rz);
        vd->tex_coord0(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position(ox + 1 * rx, oy + y * ry, oz + 1 * rz);
        vd->tex_coord0(0, 1);
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

    auto spec = sm->vertex_data->vertex_specification();
    sm->vertex_data->move_to_end();

    //Build some shared vertex data
    sm->vertex_data->position(x_offset + (-width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(0.0, 0.0f);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(0.0, 0.0f);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->move_next();

    sm->vertex_data->position(x_offset + (width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(1.0, 0.0f);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(1.0, 0.0f);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->move_next();

    sm->vertex_data->position(x_offset + (width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(1.0, 1.0f);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(1.0, 1.0f);
    if(spec.has_normals())   sm->vertex_data->normal(0, 0, 1);
    sm->vertex_data->move_next();

    sm->vertex_data->position(x_offset + (-width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    if(spec.has_diffuse())   sm->vertex_data->diffuse(smlt::Colour::WHITE);
    if(spec.has_texcoord0()) sm->vertex_data->tex_coord0(0.0, 1.0f);
    if(spec.has_texcoord1()) sm->vertex_data->tex_coord1(0.0, 1.0f);
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

void Mesh::destroy_submesh(const std::string& name) {
    auto it = std::find_if(submeshes_.begin(), submeshes_.end(), [name](const std::shared_ptr<SubMesh>& i) {
        return i->name() == name;
    });

    if(it != submeshes_.end()) {
        auto submesh = (*it);
        submeshes_.erase(it);
        signal_submesh_destroyed_(id(), submesh.get());
        aabb_dirty_ = true;
    }
}

SubMesh* Mesh::first_submesh() const {
    if(submeshes_.empty()) {
        return nullptr;
    }

    return submeshes_.front().get();
}

void Mesh::set_material(MaterialPtr material) {

    for(auto submesh: submeshes_) {
        submesh->set_material(material);
    }
}

void Mesh::transform_vertices(const smlt::Mat4& transform) {
    vertex_data->move_to_start();

    for(uint32_t i = 0; i < vertex_data->count(); ++i) {
        if(vertex_data->vertex_specification().has_positions()) {
            auto v = vertex_data->position_at<Vec3>(i);
            vertex_data->position(v->transformed_by(transform));
        }

        if(vertex_data->vertex_specification().has_normals()) {
            auto n = vertex_data->normal_at<Vec3>(i);
            vertex_data->normal(n->rotated_by(transform).normalized());
        }
        vertex_data->move_next();
    }
    vertex_data->done();
}

SubMeshIteratorPair Mesh::each_submesh() const {
    return SubMeshIteratorPair(submeshes_);
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
    float scaling = 1.0f / dia;

    smlt::Mat4 scale_matrix = Mat4::as_scaling(scaling);

    transform_vertices(scale_matrix);
}

void Mesh::reverse_winding() {
    for(auto submesh: submeshes_) {
        submesh->reverse_winding();
    }
}

SubMesh* Mesh::find_submesh(const std::string& name) const {
    auto it = std::find_if(submeshes_.begin(), submeshes_.end(), [name](const std::shared_ptr<SubMesh>& i) {
        return i->name() == name;
    });

    if(it != submeshes_.end()) {
        return it->get();
    }

    return nullptr;
}

void Mesh::generate_adjacency_info() {
    adjacency_.reset(new AdjacencyInfo(this));
    adjacency_->rebuild();
}

}
