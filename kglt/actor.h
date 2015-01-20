#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"
#include "generic/protected_ptr.h"

#include <kazbase/signals.h>

#include "interfaces.h"
#include "object.h"
#include "mesh.h"
#include "sound.h"

#include "physics/responsive_body.h"
#include "utils/parent_setter_mixin.h"

namespace kglt {

class SubActor;

class Actor :
    public MeshInterface,
    public virtual BoundableEntity,
    public Managed<Actor>,
    public generic::Identifiable<ActorID>,
    public ParentSetterMixin<Object>,
    public Source,
    public Protectable {

public:
    Actor(Stage* stage, ActorID id);
    Actor(Stage* stage, ActorID id, MeshID mesh);

    MeshID mesh_id() const { return (mesh_) ? mesh_->id() : MeshID(0); }

    ProtectedPtr<Mesh> mesh() const;

    bool has_mesh() const { return bool(mesh_); }
    void set_mesh(MeshID mesh);

    const VertexData& shared_data() const;

    const uint16_t subactor_count() const {
        return subactors_.size();
    }

    void override_material_id(MaterialID mat);

    SubActor& subactor(uint16_t idx) {
        return *subactors_[idx];
    }

    const std::vector<std::shared_ptr<SubActor> >& _subactors() { return subactors_; }

    sig::signal<void (ActorID)>& signal_mesh_changed() { return signal_mesh_changed_; }

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
private:
    std::shared_ptr<Mesh> mesh_;
    std::vector<std::shared_ptr<SubActor> > subactors_;

    RenderPriority render_priority_;

    sig::signal<void (ActorID)> signal_mesh_changed_;

    void do_update(double dt) {
        update_source(dt);
    }

    std::shared_ptr<ResponsiveBody> body_;

    void rebuild_subactors();
    sig::connection submeshes_changed_connection_;

    friend class SubActor;
};

class SubActor :
    public SubMeshInterface,
    public virtual BoundableEntity,
    public Managed<SubActor>,
    public Renderable {

public:
    const MaterialID material_id() const;

    const SubMeshIndex submesh_id() const;

    void override_material_id(MaterialID material);

    const VertexData& vertex_data() const { return submesh().vertex_data(); }
    const IndexData& index_data() const { return submesh().index_data(); }
    const MeshArrangement arrangement() const { return submesh().arrangement(); }

    void _update_vertex_array_object() { submesh()._update_vertex_array_object(); }
    void _bind_vertex_array_object() { submesh()._bind_vertex_array_object(); }

    RenderPriority render_priority() const { return parent_.render_priority(); }
    Mat4 final_transformation() const { return parent_.absolute_transformation(); }
    const bool is_visible() const { return parent_.is_visible(); }

    MeshID instanced_mesh_id() const { return parent_.mesh_id(); }
    SubMeshIndex instanced_submesh_id() const { return submesh_id(); }

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
        return submesh().aabb();
    }

    SubActor(Actor& parent, SubMesh* submesh):
        parent_(parent),
        submesh_(submesh),
        material_(0) {
    }

private:
    Actor& parent_;
    SubMesh* submesh_ = nullptr;
    MaterialPtr material_;

    SubMesh& submesh();
    const SubMesh& submesh() const;
};

}

#endif // ENTITY_H
