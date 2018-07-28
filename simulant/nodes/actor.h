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

#ifndef ENTITY_H
#define ENTITY_H

#include "../generic/identifiable.h"
#include "../generic/managed.h"

#include "../deps/kazsignal/kazsignal.h"

#include "stage_node.h"
#include "../interfaces.h"
#include "../meshes/mesh.h"
#include "../sound.h"

#include "../renderers/batching/render_queue.h"
#include "../renderers/batching/renderable.h"

namespace smlt {

class KeyFrameAnimationState;
class SubActor;

class Actor :
    public StageNode,
    public virtual Boundable,
    public Managed<Actor>,
    public generic::Identifiable<ActorID>,
    public Source,
    public HasMutableRenderPriority {

public:
    Actor(ActorID id, Stage* stage, SoundDriver *sound_driver);
    Actor(ActorID id, Stage* stage, SoundDriver *sound_driver, MeshID mesh);
    virtual ~Actor();

    const AABB& aabb() const override;

    MeshID mesh_id() const { return (mesh_) ? mesh_->id() : MeshID(0); }

    MeshPtr mesh() const;

    bool has_mesh() const { return bool(mesh_); }
    void set_mesh(MeshID mesh);

    const uint16_t subactor_count() const {
        return subactors_.size();
    }

    void override_material_id(MaterialID mat);
    void remove_material_id_override();

    SubActor& subactor(uint16_t idx) {
        return *subactors_[idx];
    }

    const std::vector<std::shared_ptr<SubActor> >& _subactors() { return subactors_; }

    void ask_owner_for_destruction() override;

    void each(std::function<void (uint32_t, SubActor*)> callback);

    typedef sig::signal<void (ActorID, SubActor*)> SubActorCreatedCallback;
    typedef sig::signal<void (ActorID, SubActor*)> SubActorDestroyedCallback;
    typedef sig::signal<void (ActorID, SubActor*, MaterialID, MaterialID)> SubActorMaterialChangedCallback;
    typedef sig::signal<void (ActorID)> MeshChangedCallback;

    SubActorCreatedCallback& signal_subactor_created() {
        return signal_subactor_created_;
    }

    SubActorDestroyedCallback& signal_subactor_destroyed() {
        return signal_subactor_destroyed_;
    }

    SubActorMaterialChangedCallback& signal_subactor_material_changed() {
        return signal_subactor_material_changed_;
    }

    MeshChangedCallback& signal_mesh_changed() { return signal_mesh_changed_; }

    void set_renderable_culling_mode(RenderableCullingMode mode) {
        culling_mode_ = mode;
    }

    RenderableCullingMode renderable_culling_mode() const { return culling_mode_; }

    Property<Actor, KeyFrameAnimationState> animation_state = { this, &Actor::animation_state_ };
    Property<Actor, VertexData> vertex_data = {this, &Actor::vertex_data_};

    bool has_animated_mesh() const {
        return mesh_ && mesh_->is_animated();
    }

    void cleanup() override {
        StageNode::cleanup();
    }

    RenderableList _get_renderables(const Frustum &frustum) const {
        return RenderableList(subactors_.begin(), subactors_.end());
    }
private:
    // Used for animated meshes
    std::unique_ptr<HardwareBuffer> interpolated_vertex_buffer_;

    VertexData* vertex_data_ = nullptr;

    std::shared_ptr<Mesh> mesh_;
    std::vector<std::shared_ptr<SubActor> > subactors_;
    std::shared_ptr<KeyFrameAnimationState> animation_state_;

    RenderableCullingMode culling_mode_ = RENDERABLE_CULLING_MODE_PARTITIONER;

    SubActorCreatedCallback signal_subactor_created_;
    SubActorDestroyedCallback signal_subactor_destroyed_;
    SubActorMaterialChangedCallback signal_subactor_material_changed_;
    MeshChangedCallback signal_mesh_changed_;

    void update(float dt) override;
    void clear_subactors();
    void rebuild_subactors();
    sig::connection submesh_created_connection_;
    sig::connection submesh_destroyed_connection_;

    friend class SubActor;

    void refresh_animation_state(uint32_t current_frame, uint32_t next_frame, float interp);
};

class SubActor :
    public SubMeshInterface,
    public virtual BoundableEntity,
    public Managed<SubActor>,
    public Renderable {

public:
    const MaterialID material_id() const;

    void override_material_id(MaterialID material);
    void remove_material_id_override();

    const MeshArrangement arrangement() const { return submesh()->arrangement(); }

    RenderPriority render_priority() const { return parent_.render_priority(); }
    Mat4 final_transformation() const { return parent_.absolute_transformation(); }
    const bool is_visible() const { return parent_.is_visible(); }

    /* BoundableAndTransformable interface implementation */

    const AABB transformed_aabb() const {
        auto corners = aabb().corners();
        auto transform = parent_.absolute_transformation();

        for(auto& corner: corners) {
            corner = corner.transformed_by(transform);
        }

        return AABB(corners.data(), corners.size());
    }

    const AABB& aabb() const {
        return submesh()->aabb();
    }

    SubActor(Actor& parent, std::shared_ptr<SubMesh> submesh);
    ~SubActor();

    SubMesh* submesh();
    const SubMesh* submesh() const;

    void prepare_buffers(Renderer* renderer);

    VertexSpecification vertex_attribute_specification() const;
    HardwareBuffer* vertex_attribute_buffer() const;
    HardwareBuffer* index_buffer() const;
    std::size_t index_element_count() const;
    IndexType index_type() const;

private:
    VertexData* get_vertex_data() const;
    IndexData* get_index_data() const;

    Actor& parent_;
    std::shared_ptr<SubMesh> submesh_;
    MaterialPtr material_;

    sig::connection submesh_material_changed_connection_;

    std::unique_ptr<HardwareBuffer> interpolated_vertex_buffer_;

};

}

#endif // ENTITY_H
