/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
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
#include "../asset.h"
#include "../vertex_data.h"
#include "../types.h"
#include "../interfaces.h"
#include "../animation.h"

namespace smlt {

class AssetManager;
class AdjacencyInfo;
class Renderer;
class Rig;
class Skeleton;
class Debug;

enum MeshAnimationType {
    MESH_ANIMATION_TYPE_NONE,
    MESH_ANIMATION_TYPE_VERTEX_MORPH,
    MESH_ANIMATION_TYPE_SKELETAL
};


typedef sig::signal<void (Mesh*, MeshAnimationType, uint32_t)> SignalAnimationEnabled;


/* When enabling animations you must pass MeshFrameData which holds all the data necessary to
 * produce a frame
 */
class FrameUnpacker {
public:
    virtual ~FrameUnpacker() {}

    /*
     * Used to interpolate the rig (if any) or do
     * any other kind of preparation during update()
     */
    virtual void prepare_unpack(
        uint32_t current_frame,
        uint32_t next_frame,
        float t, Rig* const rig,
        Debug* const debug=nullptr
    ) = 0;

    /* Used before rendering to generate the output
     * vertices with the given Rig (if any) and interpolated
     * value */
    virtual void unpack_frame(
        const uint32_t current_frame,
        const uint32_t next_frame,
        const float t,
        Rig* const rig,
        VertexData* const out,
        Debug* const debug=nullptr
    ) = 0;
};

typedef std::shared_ptr<FrameUnpacker> FrameUnpackerPtr;


class SubMeshIteratorPair {
public:
    typedef std::vector<std::shared_ptr<SubMesh>> container_type;
    typedef typename container_type::iterator iterator_type;

private:
    /* We copy the container because submeshes are often
     * altered in a loop, and as they're shared pointers
     * we can copy the container and maintain the submeshes
     * for the lifetime of the iterator */

    friend class Mesh;
    SubMeshIteratorPair(const container_type& container):
        container_(container) {

    }

    container_type container_;

public:
    iterator_type begin() {
        return container_.begin();
    }

    iterator_type end() {
        return container_.end();
    }
};

class Mesh :
    public virtual Boundable,
    public Asset,
    public Loadable,
    public RefCounted<Mesh>,
    public generic::Identifiable<MeshID>,
    public KeyFrameAnimated,
    public ChainNameable<Mesh> {

    DEFINE_SIGNAL(SignalAnimationEnabled, signal_animation_enabled);

public:
    /**
     *  Construct a mesh using a shared VertexData instance.
     *
     *  This is useful for having different resolutions of mesh (e.g.
     *  same vertex data, different indices)
     */
    Mesh(
        MeshID id,
        AssetManager* asset_manager,
        VertexDataPtr vertex_data
    );

    /**
     *  Construct a mesh using a VertexSpecification.
     *
     *  This creates a unique VertexData instance for this mesh.
     */
    Mesh(
        MeshID id,
        AssetManager* asset_manager,
        VertexSpecification vertex_specification
    );

    virtual ~Mesh();

    void reset(VertexDataPtr vertex_data);
    void reset(VertexSpecification vertex_specification);

    /* Add a skeleton to this mesh, returns False if
     * the mesh already had a skeleton, otherwise returns true */
    bool add_skeleton(uint32_t num_joints);

    /* Returns true if the Mesh has had a skeleton added */
    bool has_skeleton() const;

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

    SubMeshPtr new_submesh_as_capsule(
        const std::string& name,
        MaterialID material,
        float diameter,
        float length,
        std::size_t segment_count,
        std::size_t vertical_segment_count,
        std::size_t ring_count
    );

    SubMeshPtr new_submesh_as_sphere(const std::string& name,
        MaterialID material,
        float diameter,
        std::size_t slices,
        std::size_t stacks
    );

    SubMeshPtr new_submesh_as_icosphere(const std::string& name,
        MaterialID material,
        float diameter,
        uint32_t subdivisions
    );

    SubMeshPtr new_submesh_as_rectangle(
        const std::string& name,
        MaterialID material,
        float width,
        float height,
        const Vec3& offset=Vec3()
    );

    SubMeshPtr new_submesh_as_cube(
        const std::string& name,
        MaterialID material,
        float size
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
    bool has_submesh(const std::string& name) const;
    SubMeshPtr find_submesh(const std::string& name) const;
    SubMeshPtr first_submesh() const;

    void destroy_submesh(const std::string& name);

    void set_material(MaterialPtr material); ///< Apply material to all submeshes
    void set_diffuse(const smlt::Colour& colour); ///< Override vertex colour on all vertices

    void reverse_winding(); ///< Reverse the winding of all submeshes

    const AABB& aabb() const;
    void normalize(); //Scales the mesh so it has a radius of 1.0
    void transform_vertices(const smlt::Mat4& transform);

    SubMeshIteratorPair each_submesh() const;

    void enable_animation(MeshAnimationType animation_type, uint32_t animation_frames, FrameUnpackerPtr data);
    bool is_animated() const { return animation_type_ != MESH_ANIMATION_TYPE_NONE; }
    uint32_t animation_frames() const { return animation_frames_; }
    MeshAnimationType animation_type() const { return animation_type_; }

    /* Generates adjacency information for this mesh. This is necessary for stencil shadowing
     * to work */
    void generate_adjacency_info();
    bool has_adjacency_info() const { return bool(adjacency_); }

public:
    // Signals

    typedef sig::signal<void (Skeleton*)> SkeletonAddedSignal;
    typedef sig::signal<void (MeshID, SubMeshPtr)> SubMeshCreatedCallback;
    typedef sig::signal<void (MeshID, SubMeshPtr)> SubMeshDestroyedCallback;
    typedef sig::signal<void (MeshID, SubMeshPtr, MaterialSlot, MaterialID, MaterialID)> SubMeshMaterialChangedCallback;

    DEFINE_SIGNAL(SkeletonAddedSignal, signal_skeleton_added);

    SubMeshCreatedCallback& signal_submesh_created() { return signal_submesh_created_; }
    SubMeshDestroyedCallback& signal_submesh_destroyed() { return signal_submesh_destroyed_; }
    SubMeshMaterialChangedCallback& signal_submesh_material_changed() { return signal_submesh_material_changed_; }

private:
    friend class SubMesh;
    friend class Actor;

    Skeleton* skeleton_ = nullptr;

    VertexDataPtr vertex_data_;
    MeshAnimationType animation_type_ = MESH_ANIMATION_TYPE_NONE;
    uint32_t animation_frames_ = 0;
    FrameUnpackerPtr animated_frame_data_;

    std::vector<std::shared_ptr<SubMesh>> submeshes_;

    SubMeshCreatedCallback signal_submesh_created_;
    SubMeshDestroyedCallback signal_submesh_destroyed_;
    SubMeshMaterialChangedCallback signal_submesh_material_changed_;

    void rebuild_aabb() const;
    mutable AABB aabb_;
    mutable bool aabb_dirty_ = true;

    /* Automatically maintain adjacency info for submeshes or not */
    bool maintain_adjacency_info_ = true;
    std::unique_ptr<AdjacencyInfo> adjacency_;

    void on_vertex_data_done();
    sig::connection done_connection_;

public:
    /* Returns a nullptr if there is no adjacecy info */
    S_DEFINE_PROPERTY(adjacency_info, &Mesh::adjacency_);

    S_DEFINE_PROPERTY(vertex_data, &Mesh::vertex_data_);

    /* Returns a nullptr if there is no skeleton */
    S_DEFINE_PROPERTY(skeleton, &Mesh::skeleton_);
};

}

#endif // NEWMESH_H
