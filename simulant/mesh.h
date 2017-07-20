/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NEWMESH_H
#define NEWMESH_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <set>
#include <memory>
#include <list>

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/property.h"

#include "interfaces/boundable.h"
#include "loadable.h"
#include "resource.h"
#include "vertex_data.h"
#include "types.h"
#include "interfaces.h"
#include "animation.h"

namespace smlt {

class HardwareBuffer;
class ResourceManager;

class MeshInterface:
    public virtual Boundable {

public:
    virtual ~MeshInterface() {}

    Property<MeshInterface, VertexData> shared_data = { this, &MeshInterface::get_shared_data };

private:
    virtual VertexData* get_shared_data() const = 0;
};

class SubMeshInterface:
    public Boundable {

public:
    virtual ~SubMeshInterface() {}

    virtual const MaterialID material_id() const = 0;
    virtual const MeshArrangement arrangement() const = 0;

    Property<SubMeshInterface, VertexData> vertex_data = { this, &SubMeshInterface::get_vertex_data };
    Property<SubMeshInterface, IndexData> index_data = { this, &SubMeshInterface::get_index_data };

private:
    virtual VertexData* get_vertex_data() const = 0;
    virtual IndexData* get_index_data() const = 0;
};

enum VertexSharingMode {
    VERTEX_SHARING_MODE_SHARED,
    VERTEX_SHARING_MODE_INDEPENDENT
};

class SubMesh :
    public SubMeshInterface,
    public Managed<SubMesh>,
    public std::enable_shared_from_this<SubMesh> {

public:
    SubMesh(
        Mesh* parent,
        const std::string& name,
        MaterialID material,
        MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES,
        VertexSharingMode vertex_sharing = VERTEX_SHARING_MODE_SHARED,
        VertexSpecification vertex_specification = VertexSpecification(),
        IndexType index_type = INDEX_TYPE_16_BIT
    );

    virtual ~SubMesh();

    const MaterialID material_id() const;
    void set_material_id(MaterialID mat);

    void set_diffuse(const smlt::Colour& colour);

    const MeshArrangement arrangement() const { return arrangement_; }

    const AABB& aabb() const {
        return bounds_;
    }

    bool uses_shared_vertices() const { return uses_shared_data_; }
    void reverse_winding();
    void transform_vertices(const Mat4 &transformation);
    void set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass=0);

    void _recalc_bounds();

    void generate_texture_coordinates_cube(uint32_t texture=0);

    VertexData* get_vertex_data() const;
    IndexData* get_index_data() const;

    const std::string& name() const { return name_; }

    HardwareBuffer* vertex_buffer() const;
    HardwareBuffer* index_buffer() const { return index_buffer_.get(); }

    void prepare_buffers(); // Called by actors to make sure things are up-to-date before rendering
public:
    typedef sig::signal<void (SubMeshPtr, MaterialID, MaterialID)> MaterialChangedCallback;

    MaterialChangedCallback& signal_material_changed() {
        return signal_material_changed_;
    }

private:
    friend class Mesh;

    sig::connection material_change_connection_;

    Mesh* parent_;
    std::string name_;

    MaterialPtr material_;
    MeshArrangement arrangement_;
    bool uses_shared_data_;

    VertexData* vertex_data_ = nullptr;
    IndexData* index_data_ = nullptr;

    std::unique_ptr<HardwareBuffer> vertex_buffer_;
    std::unique_ptr<HardwareBuffer> index_buffer_;

    bool vertex_buffer_dirty_ = false;
    bool index_buffer_dirty_ = false;

    AABB bounds_;

    sig::connection vrecalc_;
    sig::connection irecalc_;

    MaterialChangedCallback signal_material_changed_;
};


enum MeshAnimationType {
    MESH_ANIMATION_TYPE_NONE,
    MESH_ANIMATION_TYPE_VERTEX_MORPH
};


typedef sig::signal<void (Mesh*, MeshAnimationType, uint32_t)> SignalAnimationEnabled;

class Mesh :
    public MeshInterface,
    public Resource,
    public Loadable,
    public Managed<Mesh>,
    public generic::Identifiable<MeshID>,
    public KeyFrameAnimated,
    public std::enable_shared_from_this<Mesh> {

    DEFINE_SIGNAL(SignalAnimationEnabled, signal_animation_enabled);

public:
    Mesh(MeshID id,
         ResourceManager* resource_manager,
         VertexSpecification vertex_specification
    );

    ~Mesh();

    void reset(VertexSpecification vertex_specification);

    SubMeshPtr new_submesh_with_material(
        const std::string& name,
        MaterialID material,        
        MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES,
        VertexSharingMode vertex_sharing=VERTEX_SHARING_MODE_SHARED,
        VertexSpecification vertex_specification=VertexSpecification(),
        IndexType=INDEX_TYPE_16_BIT
    );

    SubMeshPtr new_submesh(
        const std::string& name,
        MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES,
        VertexSharingMode vertex_sharing=VERTEX_SHARING_MODE_SHARED,
        VertexSpecification vertex_specification=VertexSpecification(),
        IndexType=INDEX_TYPE_16_BIT
    );

    SubMeshPtr new_submesh_as_rectangle(
        const std::string& name,
        MaterialID material,
        float width,
        float height,
        const Vec3& offset=Vec3()
    );

    SubMeshPtr new_submesh_as_box(
        const std::string& name,
        MaterialID material,
        float width,
        float height,
        float depth,
        const Vec3& offset=Vec3()
    );

    uint32_t submesh_count() const { return submeshes_.size(); }
    bool has_submesh(const std::string& name) const { return submeshes_.count(name); }
    SubMeshPtr submesh(const std::string& name);
    SubMeshPtr first_submesh() const;

    void delete_submesh(const std::string& name);
    void clear();

    void enable_debug(bool value);

    void set_material_id(MaterialID material); ///< Apply material to all submeshes
    void set_diffuse(const smlt::Colour& colour, bool include_submeshes=true); ///< Override vertex colour on all vertices

    void reverse_winding(); ///< Reverse the winding of all submeshes
    void set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass=0); ///< Replace the texture unit on all submesh materials

    const AABB& aabb() const;
    void normalize(); //Scales the mesh so it has a radius of 1.0
    void transform_vertices(const smlt::Mat4& transform, bool include_submeshes=true);

    void each(std::function<void (const std::string&, SubMeshPtr)> func) const;

    void enable_animation(MeshAnimationType animation_type, uint32_t animation_frames);
    bool is_animated() const { return animation_type_ != MESH_ANIMATION_TYPE_NONE; }
    uint32_t animation_frames() const { return animation_frames_; }
    MeshAnimationType animation_type() const { return animation_type_; }

    void prepare_buffers();
public:
    // Signals

    typedef sig::signal<void (MeshID, SubMeshPtr)> SubMeshCreatedCallback;
    typedef sig::signal<void (MeshID, SubMeshPtr)> SubMeshDestroyedCallback;
    typedef sig::signal<void (MeshID, SubMeshPtr, MaterialID, MaterialID)> SubMeshMaterialChangedCallback;

    SubMeshCreatedCallback& signal_submesh_created() { return signal_submesh_created_; }
    SubMeshDestroyedCallback& signal_submesh_destroyed() { return signal_submesh_destroyed_; }
    SubMeshMaterialChangedCallback& signal_submesh_material_changed() { return signal_submesh_material_changed_; }

private:
    friend class SubMesh;
    VertexData* get_shared_data() const;

    VertexData* shared_data_ = nullptr;
    MeshAnimationType animation_type_ = MESH_ANIMATION_TYPE_NONE;
    uint32_t animation_frames_ = 0;

    std::unique_ptr<HardwareBuffer> shared_vertex_buffer_;
    bool shared_vertex_buffer_dirty_ = false;

    std::unordered_map<std::string, std::shared_ptr<SubMesh>> submeshes_;
    std::list<SubMeshPtr> ordered_submeshes_; // Ordered by insertion order

    SubMeshPtr normal_debug_mesh_ = nullptr;

    SubMeshCreatedCallback signal_submesh_created_;
    SubMeshDestroyedCallback signal_submesh_destroyed_;
    SubMeshMaterialChangedCallback signal_submesh_material_changed_;

    void rebuild_aabb() const;
    mutable AABB aabb_;
    mutable bool aabb_dirty_ = true;
};

}

#endif // NEWMESH_H
