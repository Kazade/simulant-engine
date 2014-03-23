#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"
#include "generic/protected_ptr.h"

#include "kazbase/signals3/signals3.hpp"

#include "boundable.h"
#include "object.h"
#include "mesh.h"
#include "sound.h"

#include "physics/responsive_body.h"
#include "utils/parent_setter_mixin.h"

namespace kglt {

class SubActor;

class Actor :
    public MeshInterface,
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
        return *subactors_.at(idx);
    }

    const std::vector<std::shared_ptr<SubActor> >& _subactors() { return subactors_; }

    sig::signal<void (ActorID)>& signal_mesh_changed() { return signal_mesh_changed_; }

    void ask_owner_for_destruction();

    RenderPriority render_priority() const { return render_priority_; }
    void set_render_priority(RenderPriority value) { render_priority_ = value;}

    unicode __unicode__() const {
        return _u("Actor {0}").format(this->id());
    }
private:
    MeshPtr mesh_;
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
    public Managed<SubActor>,
    public Boundable {

public:
    SubActor(Actor& parent, SubMeshIndex idx):
        parent_(parent),
        index_(idx),
        material_(0) {
    }

    const MaterialID material_id() const;

    const SubMeshIndex submesh_id() const { return index_; }

    void override_material_id(MaterialID material);

    const VertexData& vertex_data() const { return submesh().vertex_data(); }
    const IndexData& index_data() const { return submesh().index_data(); }
    const MeshArrangement arrangement() const { return submesh().arrangement(); }

    void _update_vertex_array_object() { submesh()._update_vertex_array_object(); }
    void _bind_vertex_array_object() { submesh()._bind_vertex_array_object(); }

    Actor& _parent() { return parent_; }

    /* Boundable interface implementation */

    /**
     * @brief absolute_bounds
     * @return the bounds of the linked submesh, transformed by the actors absolute
     * transformation matrix.
     */
    const kmAABB absolute_bounds() const {
        kmAABB local = local_bounds();
        kmMat4 transform = parent_.absolute_transformation();

        //Transform local by the transformation matrix of the parent
        kmVec3Transform(&local.min, &local.min, &transform);
        kmVec3Transform(&local.max, &local.max, &transform);

        return local;
    }

    /**
     * @brief local_bounds
     * @return the bounds of the associated submesh
     */
    const kmAABB local_bounds() const {
        return submesh().bounds();
    }

    const Vec3 centre() const {
        // Return the centre point of the absolute bounds of this subactor
        // which is the submesh().bounds() transformed by the parent actor's
        // location
        Vec3 centre;
        kmAABB abs_bounds = absolute_bounds();
        kmAABBCentre(&abs_bounds, &centre);
        return centre;
    }

private:
    Actor& parent_;
    SubMeshIndex index_;
    MaterialPtr material_;

    SubMesh& submesh();
    const SubMesh& submesh() const;
};

}

#endif // ENTITY_H
