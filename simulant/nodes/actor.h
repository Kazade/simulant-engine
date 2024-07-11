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

#ifndef ENTITY_H
#define ENTITY_H

#include "../generic/identifiable.h"
#include "../generic/managed.h"

#include "../signals/signal.h"

#include "../generic/manual_object.h"
#include "../interfaces.h"
#include "../meshes/mesh.h"
#include "../sound.h"
#include "simulant/utils/params.h"
#include "stage_node.h"

#include "../renderers/batching/render_queue.h"
#include "../renderers/batching/renderable.h"

namespace smlt {

class KeyFrameAnimationState;
class Rig;

struct ActorParams {
    ActorParams(MeshPtr mesh) :
        mesh(mesh) {}

    MeshPtr mesh;
};

class Actor:
    public StageNode,
    public virtual Boundable,
    public HasMutableRenderPriority,
    public ChainNameable<Actor> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_ACTOR, "actor");
    S_DEFINE_STAGE_NODE_PARAM(Actor, "mesh", MeshPtr, no_value,
                              "The base mesh associated with this actor");

    Actor(Scene* owner);
    virtual ~Actor();

    const AABB& aabb() const override;

    AssetID mesh_id(DetailLevel detail_level) const;
    const MeshPtr& mesh(DetailLevel detail_level) const;
    const MeshPtr& best_mesh(DetailLevel detail_level) const;
    const MeshPtr& base_mesh() const;

    bool has_mesh(DetailLevel detail_level) const;
    bool has_any_mesh() const;
    bool has_multiple_meshes() const;

    void set_mesh(const MeshPtr& mesh,
                  DetailLevel detail_level = DETAIL_LEVEL_NEAREST);

    typedef sig::signal<void(StageNodeID)> MeshChangedCallback;

    MeshChangedCallback& signal_mesh_changed() {
        return signal_mesh_changed_;
    }

    /* Returns true if the nearest detail level mesh is animated. */
    bool has_animated_mesh() const {
        return has_animated_mesh_;
    }

    void do_generate_renderables(batcher::RenderQueue* render_queue,
                                 const Camera* camera, const Viewport* viewport,
                                 const DetailLevel detail_level) override;

    void use_material_slot(MaterialSlot var) {
        material_slot_ = var;
    }

    MaterialSlot active_material_slot() const {
        return material_slot_;
    }

    /*
     * Returns true if the attached base mesh has a skeleton
     * and so can be overridden by the rig
     */
    bool is_rigged() const {
        return bool(rig_);
    }

private:
    bool on_create(Params params) override;

    const MeshPtr& find_mesh(DetailLevel level) const {
        /* Find the most suitable mesh at the specified level. This will search
         * downwards from the level to NEAREST and return the first non-null
         * result */
        return effective_meshes_[level];
    }

    // Used for animated meshes
    std::shared_ptr<VertexData> interpolated_vertex_data_;

    /* Meshes specified for each level */
    MeshPtr meshes_[DETAIL_LEVEL_MAX];

    /* Quick lookup for which mesh is active at a detail level */
    MeshPtr effective_meshes_[DETAIL_LEVEL_MAX];

    void recalc_effective_meshes();

    bool has_animated_mesh_ = false;

    std::shared_ptr<KeyFrameAnimationState> animation_state_;

    MeshChangedCallback signal_mesh_changed_;

    MaterialSlot material_slot_ = MATERIAL_SLOT0;

    void on_update(float dt) override;

    sig::connection submesh_created_connection_;
    sig::connection submesh_destroyed_connection_;

    void refresh_animation_state(uint32_t current_frame, uint32_t next_frame,
                                 float interp);

    /* Only available if the base mesh has a skeleton */
    std::unique_ptr<Rig> rig_;
    void add_rig(const Skeleton* skeleton);
    sig::connection mesh_skeleton_added_;

public:
    S_DEFINE_PROPERTY(animation_state, &Actor::animation_state_);
    S_DEFINE_PROPERTY(rig, &Actor::rig_);
};

} // namespace smlt

#endif // ENTITY_H
