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
#include "../window.h"
#include "../resource_manager.h"
#include "../loader.h"
#include "../material.h"
#include "../hardware_buffer.h"
#include "../renderers/renderer.h"


namespace smlt {

VertexData *SubMesh::get_vertex_data() const {
    return parent_->shared_data.get();
}

IndexData* SubMesh::get_index_data() const {
    return index_data_;
}

HardwareBuffer* SubMesh::vertex_buffer() const {
    return parent_->shared_vertex_buffer_.get();
}

Mesh::Mesh(MeshID id,
    ResourceManager *resource_manager,
    VertexSpecification vertex_specification):
        Resource(resource_manager),
        generic::Identifiable<MeshID>(id) {

    reset(vertex_specification);

    shared_data_->signal_update_complete().connect([this]() {
        // Mark the AABB as dirty so it will be rebuilt on next access
        aabb_dirty_ = true;
    });
}

template<typename Data, typename Allocator>
void sync_buffer(HardwareBuffer::ptr* buffer, Data* data, Allocator* allocator, HardwareBufferPurpose purpose) {
    if(!(*buffer) && data->count()) {
        (*buffer) = allocator->hardware_buffers->allocate(
            data->data_size(),
            purpose,
            SHADOW_BUFFER_DISABLED
        );
    } else {
        assert(data->count());
        (*buffer)->resize(data->data_size());
    }

    (*buffer)->upload(*data);
}

void Mesh::reset(VertexSpecification vertex_specification) {
    submeshes_.clear();
    ordered_submeshes_.clear();

    animation_type_ = MESH_ANIMATION_TYPE_NONE;
    animation_frames_ = 0;

    delete shared_data_;
    shared_data_ = new VertexData(vertex_specification);

    // When the vertex data updates, update the hardware buffer
    shared_data->signal_update_complete().connect([this]() { shared_vertex_buffer_dirty_ = true; });
}

Mesh::~Mesh() {
    assert(ordered_submeshes_.size() == submeshes_.size());

    delete shared_data_;
    shared_data_ = nullptr;
}

void Mesh::each(std::function<void (const std::string&, SubMesh*)> func) const {
    assert(ordered_submeshes_.size() == submeshes_.size());

    // Respect insertion order while iterating
    for(SubMesh* submesh: this->ordered_submeshes_) {
        func(submesh->name(), submesh);
    }
}

void Mesh::clear() {
    //Delete the submeshes and clear the shared data
    submeshes_.clear();
    shared_data->clear();
    rebuild_aabb();
}

void Mesh::enable_animation(MeshAnimationType animation_type, uint32_t animation_frames) {
    if(animation_type_ != MESH_ANIMATION_TYPE_NONE) {
        throw std::logic_error("Tried to re-enable animations on an animated mesh");
    }

    if(!animation_frames) {
        throw std::logic_error("You must specify the number of frames when enabling mesh animations");
    }

    animation_type_ = animation_type;
    animation_frames_ = animation_frames;

    signal_animation_enabled_(this, animation_type_, animation_frames_);
}

VertexData* Mesh::get_shared_data() const {
    return shared_data_;
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

SubMesh* Mesh::new_submesh_as_box(const std::string& name, MaterialID material, float width, float height, float depth, const Vec3& offset) {
    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_3F;
    spec.normal_attribute = VERTEX_ATTRIBUTE_3F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.texcoord1_attribute = VERTEX_ATTRIBUTE_2F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;

    SubMesh* sm = new_submesh_with_material(
        name,
        material,
        MESH_ARRANGEMENT_TRIANGLES
    );

    auto vd = sm->vertex_data.get();
    auto id = sm->index_data.get();

    float x_offset = offset.x;
    float y_offset = offset.y;
    float z_offset = offset.z;

    float rx = width * 0.5f;
    float ry = height * 0.5f;
    float rz = depth * 0.5f;

    //front and back
    for(int32_t z: { -1, 1 }) {
        uint32_t count = vd->count();

        vd->position(-1 * rx, -1 * ry, z * rz);
        vd->tex_coord0(0, 0);
        vd->tex_coord1(0, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position( 1 * rx, -1 * ry, z * rz);
        vd->tex_coord0(1, 0);
        vd->tex_coord1(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position( 1 * rx,  1 * ry, z * rz);
        vd->tex_coord0(1, 1);
        vd->tex_coord1(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, 0, z);
        vd->move_next();

        vd->position(-1 * rx,  1 * ry, z * rz);
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

        vd->position( x * rx, -1 * ry, -1 * rz);
        vd->tex_coord0(0, 0);
        vd->tex_coord1(0, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position( x * rx,  1 * ry, -1 * rz);
        vd->tex_coord0(1, 0);
        vd->tex_coord1(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position( x * rx,  1 * ry, 1 * rz);
        vd->tex_coord0(1, 1);
        vd->tex_coord1(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(x, 0, 0);
        vd->move_next();

        vd->position(x * rx, -1 * ry, 1 * rz);
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

        vd->position( 1 * rx, y * ry, -1 * rz);
        vd->tex_coord0(0, 0);
        vd->tex_coord1(0, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position( -1 * rx,  y * ry, -1 * rz);
        vd->tex_coord0(1, 0);
        vd->tex_coord1(1, 0);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position( -1 * rx,  y * ry, 1 * rz);
        vd->tex_coord0(1, 1);
        vd->tex_coord1(1, 1);
        vd->diffuse(smlt::Colour::WHITE);
        vd->normal(0, y, 0);
        vd->move_next();

        vd->position( 1 * rx, y * ry, 1 * rz);
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

    // Apply offset
    vd->move_to_start();
    for(uint16_t i = 0; i < vd->count(); ++i) {
        Vec3 pos = vd->position_at<Vec3>(i);
        vd->position(pos + smlt::Vec3(x_offset, y_offset, z_offset));
        vd->move_next();
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
    shared_data->move_to_start();

    for(uint32_t i = 0; i < shared_data->count(); ++i) {
        if(shared_data->specification().has_positions()) {
            smlt::Vec3 v = shared_data->position_at<Vec3>(i);
            v = v.transformed_by(transform);
            shared_data->position(v);
        }

        if(shared_data->specification().has_normals()) {
            smlt::Vec3 n;
            shared_data->normal_at(i, n);
            n = n.rotated_by(transform);
            shared_data->normal(n.normalized());
        }
        shared_data->move_next();
    }
    shared_data->done();
}

void Mesh::set_diffuse(const smlt::Colour& colour) {
    shared_data->move_to_start();
    for(uint32_t i = 0; i < shared_data->count(); ++i) {
        shared_data->diffuse(colour);
        shared_data->move_next();
    }
    shared_data->done();
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

void Mesh::set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass) {
    each([=](const std::string& name, SubMesh* mesh) {
        mesh->set_texture_on_material(unit, tex, pass);
    });
}

SubMesh* Mesh::submesh(const std::string& name) {
    auto it = submeshes_.find(name);
    if(it != submeshes_.end()) {
        return it->second.get();
    }

    return nullptr;
}

SubMesh::SubMesh(Mesh* parent, const std::string& name,
        MaterialID material, MeshArrangement arrangement, IndexType index_type):
    parent_(parent),
    name_(name),
    arrangement_(arrangement) {

    index_data_ = new IndexData(index_type);
    index_data_->signal_update_complete().connect([this]() { this->index_buffer_dirty_ = true; });

    if(!material) {
        //Set the material to the default one (store the pointer to increment the ref-count)
        material = parent_->resource_manager().clone_default_material();
    }

    set_material_id(material);

    vrecalc_ = vertex_data->signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));
    irecalc_ = index_data->signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));
}

const MaterialID SubMesh::material_id() const {
    return material_->id();
}

void Mesh::prepare_buffers() {
    if(shared_vertex_buffer_dirty_) {
        sync_buffer<VertexData, Renderer>(
            &shared_vertex_buffer_, shared_data_,
            resource_manager().window->renderer.get(),
            HARDWARE_BUFFER_VERTEX_ATTRIBUTES
        );
        shared_vertex_buffer_dirty_ = false;
    }
}

void SubMesh::prepare_buffers() {
    auto* renderer = parent_->resource_manager().window->renderer.get();

    parent_->prepare_buffers();

    if(index_buffer_dirty_) {
        sync_buffer<IndexData, Renderer>(
            &index_buffer_, index_data_,
            renderer,
            HARDWARE_BUFFER_VERTEX_ARRAY_INDICES
        );
        index_buffer_dirty_ = false;
    }
}

void SubMesh::set_diffuse(const smlt::Colour& colour) {
    index_data->each([this, colour](uint32_t i) {
        vertex_data->move_to(i);
        vertex_data->diffuse(colour);
    });

    vertex_data->done();
}

void SubMesh::set_material_id(MaterialID mat) {
    auto old_material = (material_) ? material_->id() : MaterialID();

    if(old_material == mat) {
        // Don't do anything, don't fire the changed signal
        return;
    }

    if(mat) {
        // Set the material, store the shared_ptr to increment the ref count
        material_ = parent_->resource_manager().material(mat);
        if(!material_) {
            throw std::runtime_error("Tried to set invalid material on submesh");
        }

        material_change_connection_ = material_->signal_material_changed().connect(
            [=](MaterialID) {
                /* FIXME: This is a hack! We want material_changed event to take some kind of event
                 * structure so we can signal different types of event changes. Here we are signaling that
                 * the material passes changed so the material itself changed - not that it we changed from
                 * one material to another. Still, we need to make sure that we trigger the signal so that the
                 * render queue updates. */
                signal_material_changed_(this, material_->id(), material_->id());
            }
        );
    } else {
        // Reset the material
        material_.reset();
    }

    signal_material_changed_(this, old_material, mat);
    parent_->signal_submesh_material_changed_(
        parent_->id(),
        this,
        old_material,
        mat
    );
}

void SubMesh::reverse_winding() {
    if(arrangement_ != MESH_ARRANGEMENT_TRIANGLES) {
        assert(0 && "Not implemented");
    }

    auto original = index_data->all();

    index_data->clear();
    for(uint32_t i = 0; i < original.size() / 3; ++i) {
        index_data->index(original[i * 3]);
        index_data->index(original[(i * 3) + 2]);
        index_data->index(original[(i * 3) + 1]);
    }
    index_data->done();
}

/**
 * @brief SubMesh::recalc_bounds
 *
 * Recalculate the bounds of the submesh. This involves interating over all of the
 * vertices that make up the submesh and so is potentially quite slow. This happens automatically
 * when index_data->done() is called.
 */
void SubMesh::_recalc_bounds() {
    //Set the min bounds to the max
    bounds_.set_min(Vec3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    ));

    //Set the max bounds to the min
    bounds_.set_max(Vec3(
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    ));

    if(!index_data->count()) {
        bounds_ = AABB();
        return;
    }

    auto vertex_count = vertex_data->count();
    for(uint16_t idx: index_data->all()) {
        if(idx >= vertex_count) continue; // Don't read outside the bounds

        Vec3 pos = vertex_data->position_at<Vec3>(idx);
        if(pos.x < bounds_.min().x) bounds_.set_min_x(pos.x);
        if(pos.y < bounds_.min().y) bounds_.set_min_y(pos.y);
        if(pos.z < bounds_.min().z) bounds_.set_min_z(pos.z);

        if(pos.x > bounds_.max().x) bounds_.set_max_x(pos.x);
        if(pos.y > bounds_.max().y) bounds_.set_max_y(pos.y);
        if(pos.z > bounds_.max().z) bounds_.set_max_z(pos.z);
    }
}

void SubMesh::set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass) {
    material_->pass(pass)->set_texture_unit(unit, tex);
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
    auto& vd = vertex_data;

    vd->move_to_start();
    for(uint16_t i = 0; i < vd->count(); ++i) {
        Vec3 v;
        vd->normal_at(i, v); // Get the vertex normal

        // Work out the component with the largest value
        float absx = fabs(v.x);
        float absy = fabs(v.y);
        float absz = fabs(v.z);

        bool x = (absx > absy && absx > absz);
        bool y = (absy > absx && absy > absz);

        // Generate a orthagonal direction vector

        smlt::Vec3 dir(0, 0, 0);

        if(x) {
            dir.x = (v.x < 0) ? -1 : 1;
        } else if(y) {
            dir.y = (v.y < 0) ? -1 : 1;
        } else {
            dir.z = (v.z < 0) ? -1 : 1;
        }

        // Create a plane at the origin with the opposite direction
        Plane plane(-dir.x, -dir.y, -dir.z, 0.0f);

        smlt::Vec3 v1 = vd->position_at<Vec3>(i) - bounds_.min();

        // Project the vertex position onto the plane
        smlt::Vec3 final = plane.project(v1);

        //Scale the final position on the plane by the size of the box
        // and subtract the lower corner so that everything is relative to 0,0,0
        // and scaled between 0 and 1
        final.x /= bounds_.width();
        final.y /= bounds_.height();
        final.z /= bounds_.depth();

        // Finally, offset the uv coordinate to the right 'square' of the cubic texture
        if(x) {
            if(dir.x >= 0) {
                final.x = 2.0 / 3.0 + (final.y / 3.0);
                final.y = 2.0 / 4.0 + (final.z / 4.0);
            } else {
                final.x = final.y / 3.0;
                final.y = 2.0 / 4.0 + (final.z / 4.0);
            }
        } else if(y) {
            if(dir.y >= 0) {
                final.x = 1.0 / 3.0 + (final.z / 3.0);
                final.y = 3.0 / 4.0 + (final.x / 4.0);
            } else {
                final.x = 1.0 / 3.0 + (final.z / 3.0);
                final.y = 1.0 / 4.0 + (final.x / 4.0);
            }
        } else {
            if(dir.z >= 0) {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = 2.0 / 4.0 + (final.y / 4.0);
            } else {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = (final.y / 4.0);
            }
        }

        switch(texture) {
            case 0: vd->tex_coord0(final.x, final.y);
                break;
            case 1: vd->tex_coord1(final.x, final.y);
                break;
            case 2: vd->tex_coord2(final.x, final.y);
                break;
            case 3: vd->tex_coord3(final.x, final.y);
                break;
            default:
                break;
        }
        vd->move_next();
    }

    vd->done();

}

SubMesh::~SubMesh() {
    vrecalc_.disconnect();
    irecalc_.disconnect();

    delete index_data_;
    index_data_ = nullptr;
}

}
