/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <unordered_map>

#include "../core/aligned_allocator.h"
#include "../meshes/mesh.h"
#include "../renderers/batching/renderable.h"
#include "stage_node.h"

namespace smlt {

class Joint;

/** The root of a skeleton, and the thing that renders what's attached to it.
 *
 *  An Armature owns two things:
 *
 *   - the skeleton, which is the tree of `Joint` nodes below it,
 *   - one or more skinned *source* meshes, which are shared, immutable
 *     assets holding the bind pose along with per-vertex joint indices and
 *     weights.
 *
 *  Unlike earlier versions of this class, posing does *not* produce a
 *  private copy of the mesh. Instead, each frame this armature computes the
 *  joint matrices for its pose and hands them to the render queue alongside
 *  the shared, unposed source mesh (see SkinningInfo in renderable.h) -
 *  actually skinning the vertices is deferred to the renderer, immediately
 *  before the geometry is submitted. This means any number of armatures can
 *  pose the same source mesh at once without each needing its own copy of
 *  the (potentially large) posed vertex buffer.
 *
 *  Skinning happens in armature space, so the pose doesn't depend on where
 *  the armature sits in the scene; moving the Armature moves the posed mesh
 *  with it. All that's required is that the joints are descendents of the
 *  armature.
 */
class Armature: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META("armature");

    /** The skinned mesh to pose. Where several meshes are bound to the same
     *  skeleton (glTF allows any number of mesh nodes to share one skin)
     *  the additional ones are passed as "mesh.1", "mesh.2" and so on. */
    S_DEFINE_STAGE_NODE_PARAM(Armature, "mesh", MeshPtr, MeshPtr(),
                              "A skinned mesh posed by this armature");

    /* Params beyond the first mesh aren't declared (there's no way to know
     * how many there are up-front) so they're read straight from the
     * unfiltered params in on_create. */
    static const char* extra_mesh_param_prefix() {
        return "mesh.";
    }

    Armature(Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    /** Binds a skinned mesh to this armature. Returns false if `source`
     *  couldn't be used (e.g. it has no skin). */
    bool add_mesh(const MeshPtr& source);

    std::size_t mesh_count() const {
        return meshes_.size();
    }

    /** The shared, immutable bind-pose mesh at `i` */
    MeshPtr source_mesh(std::size_t i = 0) const;

    /** Computes the posed vertices of the mesh at `i` into `out`, using the
     *  current pose. This is a synchronous CPU readback intended for tools,
     *  tests, and gameplay code that needs to inspect posed geometry (e.g.
     *  picking); the render queue does not use this and instead skins
     *  directly into a transient renderer-owned buffer. `out` is resized and
     *  its position/normal attributes overwritten; everything else is left
     *  untouched. Returns false if there's no mesh at `i` or the pose isn't
     *  up to date and couldn't be resolved. */
    bool read_back_pose(std::size_t i, VertexData& out) const;

    /** Recalculates the joint matrices of every bound mesh from the current
     *  transforms of this armature's joints. */
    void update_skinning();

    /** Flags that the pose has changed and the joint matrices need
     *  rebuilding before they're next rendered. */
    void mark_skinning_dirty() {
        skinning_dirty_ = true;
    }

    /** Flags that the set of joints below this armature has changed. */
    void mark_joints_dirty() {
        joints_dirty_ = true;
        skinning_dirty_ = true;
    }

    std::size_t joint_count() const {
        ensure_joints();
        return joints_.size();
    }

    /** The joint at `index`, or nullptr if there isn't one. Indexes match
     *  the order of the source mesh's inverse bind matrices. */
    Joint* joint(std::size_t index) const;

    /** Finds a joint by name, or nullptr */
    Joint* find_joint(const std::string& name) const;

    const AABB& aabb() const override;

    void do_generate_renderables(batcher::RenderQueue* render_queue,
                                 const Camera* camera, const Viewport* viewport,
                                 const DetailLevel detail_level, Light** lights,
                                 const std::size_t light_count,
                                 bool respect_visibility = true) override;

    void use_material_slot(MaterialSlot var) {
        material_slot_ = var;
    }

    MaterialSlot active_material_slot() const {
        return material_slot_;
    }

private:
    struct SkinnedMesh {
        MeshPtr source; ///< Shared bind-pose mesh
    };

    /* Per-Skin joint matrices, kept across frames so meshes with distinct
     * skins bound to the same armature don't clobber each other's matrices
     * (several meshes can share a Skin, in which case they share an entry
     * here too). Renderable::skinning points at `info` directly, so it must
     * stay at a stable address for as long as this armature (and the map
     * entry) lives - std::unordered_map guarantees that across insertions. */
    struct SkinPose {
        std::vector<Mat4, aligned_allocator<Mat4, 32>> joint_matrices;
        SkinningInfo info;
    };

    bool on_create(Params params) override;
    void on_late_update(float dt) override;

    /* Rebuilds joints_ by index from the Joint nodes below this armature.
     * Joints are created (and reparented) after the armature itself, so
     * this is resolved lazily rather than up-front. */
    void ensure_joints() const;

    /* pose.joint_matrices[h] = armature_world_inverse * joint_world * ibm[h],
     * with entries for missing joints left as Mat4::zero() so they can be
     * blended in unconditionally. Bumps pose.info.generation. */
    void update_joint_matrices(const Mat4& armature_world_inverse,
                               const Mesh::Skin& skin);

    void rebuild_aabb();

    std::vector<SkinnedMesh> meshes_;

    /* Mutable so that the const accessors above can resolve the skeleton on
     * first use */
    mutable std::vector<Joint*> joints_;
    mutable bool joints_dirty_ = true;

    std::unordered_map<const Mesh::Skin*, SkinPose> skin_poses_;

    bool skinning_dirty_ = true;

    AABB aabb_;

    MaterialSlot material_slot_ = MATERIAL_SLOT0;
};

typedef Armature* ArmaturePtr;

} // namespace smlt
