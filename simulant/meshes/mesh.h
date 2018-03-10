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

#include "submesh.h"

#include "../interfaces/boundable.h"
#include "../generic/managed.h"
#include "../generic/identifiable.h"
#include "../generic/property.h"

#include "../loadable.h"
#include "../resource.h"
#include "../vertex_data.h"
#include "../types.h"
#include "../interfaces.h"
#include "../animation.h"

namespace smlt {

class HardwareBuffer;
class ResourceManager;
class AdjacencyInfo;
class Renderer;

enum MeshAnimationType {
    MESH_ANIMATION_TYPE_NONE,
    MESH_ANIMATION_TYPE_VERTEX_MORPH
};


typedef sig::signal<void (Mesh*, MeshAnimationType, uint32_t)> SignalAnimationEnabled;

class Mesh :
    public virtual Boundable,
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
        IndexType=INDEX_TYPE_16_BIT
    );

    SubMeshPtr new_submesh(
        const std::string& name,
        MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES,
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

    void set_material_id(MaterialID material); ///< Apply material to all submeshes
    void set_diffuse(const smlt::Colour& colour); ///< Override vertex colour on all vertices

    void reverse_winding(); ///< Reverse the winding of all submeshes
    void set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass=0); ///< Replace the texture unit on all submesh materials

    const AABB& aabb() const;
    void normalize(); //Scales the mesh so it has a radius of 1.0
    void transform_vertices(const smlt::Mat4& transform);

    // DEPRECATED use each_submesh
    void each(std::function<void (const std::string&, SubMeshPtr)> func) const;
    void each_submesh(std::function<void (const std::string&, SubMeshPtr)> func) const;

    void enable_animation(MeshAnimationType animation_type, uint32_t animation_frames);
    bool is_animated() const { return animation_type_ != MESH_ANIMATION_TYPE_NONE; }
    uint32_t animation_frames() const { return animation_frames_; }
    MeshAnimationType animation_type() const { return animation_type_; }

    void prepare_buffers(Renderer *renderer);

    /* Generates adjacency information for this mesh. This is necessary for stencil shadowing
     * to work */
    void generate_adjacency_info();
    bool has_adjacency_info() const { return bool(adjacency_); }

    /* Returns a nullptr if there is no adjacecy info */
    Property<Mesh, AdjacencyInfo> adjacency_info = {this, &Mesh::adjacency_};
    Property<Mesh, VertexData> vertex_data = { this, &Mesh::vertex_data_ };
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

    VertexData* vertex_data_ = nullptr;
    MeshAnimationType animation_type_ = MESH_ANIMATION_TYPE_NONE;
    uint32_t animation_frames_ = 0;

    std::unique_ptr<HardwareBuffer> shared_vertex_buffer_;
    bool shared_vertex_buffer_dirty_ = false;

    std::unordered_map<std::string, std::shared_ptr<SubMesh>> submeshes_;
    std::list<SubMeshPtr> ordered_submeshes_; // Ordered by insertion order

    SubMeshCreatedCallback signal_submesh_created_;
    SubMeshDestroyedCallback signal_submesh_destroyed_;
    SubMeshMaterialChangedCallback signal_submesh_material_changed_;

    void rebuild_aabb() const;
    mutable AABB aabb_;
    mutable bool aabb_dirty_ = true;

    /* Automatically maintain adjacency info for submeshes or not */
    bool maintain_adjacency_info_ = true;
    std::unique_ptr<AdjacencyInfo> adjacency_;
};

}

#endif // NEWMESH_H
