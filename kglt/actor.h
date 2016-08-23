#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"

#include <kazsignal/kazsignal.h>

#include "interfaces.h"
#include "object.h"
#include "mesh.h"
#include "sound.h"

#include "utils/parent_setter_mixin.h"
#include "renderers/batching/render_queue.h"
#include "renderers/batching/renderable.h"

namespace kglt {

class SubActor;

class Actor :
    public MeshInterface,
    public virtual BoundableEntity,
    public Managed<Actor>,
    public generic::Identifiable<ActorID>,
    public ParentSetterMixin<MoveableObject>,
    public Source {

public:
    Actor(ActorID id, Stage* stage);
    Actor(ActorID id, Stage* stage, MeshID mesh);

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

    void ask_owner_for_destruction();

    RenderPriority render_priority() const { return render_priority_; }
    void set_render_priority(RenderPriority value) { render_priority_ = value;}

    unicode __unicode__() const {
        if(has_name()) {
            return name();
        } else {
            return _u("Actor {0}").format(this->id());
        }
    }

    const AABB aabb() const;
    const AABB transformed_aabb() const;

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

private:
    VertexData* get_shared_data() const;

    std::shared_ptr<Mesh> mesh_;
    std::vector<std::shared_ptr<SubActor> > subactors_;

    RenderPriority render_priority_;
    RenderableCullingMode culling_mode_ = RENDERABLE_CULLING_MODE_PARTITIONER;

    SubActorCreatedCallback signal_subactor_created_;
    SubActorDestroyedCallback signal_subactor_destroyed_;
    SubActorMaterialChangedCallback signal_subactor_material_changed_;
    MeshChangedCallback signal_mesh_changed_;

    void do_update(double dt) {
        update_source(dt);
    }

    void clear_subactors();
    void rebuild_subactors();
    sig::connection submesh_created_connection_;
    sig::connection submesh_destroyed_connection_;

    friend class SubActor;
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

#ifdef KGLT_GL_VERSION_2X
    void _update_vertex_array_object() { submesh()->_update_vertex_array_object(); }
    void _bind_vertex_array_object() { submesh()->_bind_vertex_array_object(); }
#endif

    RenderPriority render_priority() const { return parent_.render_priority(); }
    Mat4 final_transformation() const { return parent_.absolute_transformation(); }
    const bool is_visible() const { return parent_.is_visible(); }

    /* BoundableAndTransformable interface implementation */

    const AABB transformed_aabb() const {
        AABB local = aabb();
        Mat4 transform = parent_.absolute_transformation();

        //Transform local by the transformation matrix of the parent
        kmVec3Transform(&local.min, &local.min, &transform);
        kmVec3Transform(&local.max, &local.max, &transform);

        return local;
    }

    const AABB aabb() const {
        return submesh()->aabb();
    }

    SubActor(Actor& parent, std::shared_ptr<SubMesh> submesh);
    ~SubActor();

    SubMesh* submesh();
    const SubMesh* submesh() const;


    /* These properties are inherited by both the SubMeshInterface and RenderableInterface
     * and both perform the same action, so we pull the SubMeshInterface ones into scope */
    using SubMeshInterface::vertex_data;
    using SubMeshInterface::index_data;
private:
    VertexData* get_vertex_data() const;
    IndexData* get_index_data() const;

    Actor& parent_;
    std::shared_ptr<SubMesh> submesh_;
    MaterialPtr material_;

    sig::connection submesh_material_changed_connection_;

};

}

#endif // ENTITY_H
