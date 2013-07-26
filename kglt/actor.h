#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"

#include "boundable.h"
#include "object.h"
#include "mesh.h"
#include "sound.h"

#include "physics/physics_body.h"

namespace kglt {

class SubActor;

class Actor :
    public MeshInterface,
    public Managed<Actor>,
    public generic::Identifiable<ActorID>,
    public Object,
    public Source {

public:
    Actor(Stage* stage, ActorID id);
    Actor(Stage* stage, ActorID id, MeshID mesh);

    MeshID mesh_id() const { return (mesh_) ? mesh_->id() : MeshID(0); }
    MeshRef mesh() const { return mesh_; }
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

    sigc::signal<void, ActorID>& signal_mesh_changed() { return signal_mesh_changed_; }

    void destroy();

    RenderPriority render_priority() const { return render_priority_; }
    void set_render_priority(RenderPriority value) { render_priority_ = value;}

    void set_body(std::shared_ptr<PhysicsBody> body) {
        body_ = body;
    }

    PhysicsBody* body() { return body_.get(); }
    const PhysicsBody* body() const { return body_.get(); }

    //Override the Object moving functions to take into account
    //the possiblity of a body
    virtual void move_to(float x, float y, float z) {
        if(!body()) {
            Object::move_to(x, y, z);
        } else {
            body()->set_position(kglt::Vec3(x, y, z));
        }
    }
    virtual void move_to(const kmVec3& pos) { move_to(pos.x, pos.y, pos.z); }

    virtual kmVec3 absolute_position() const {
        if(!body()) {
            return Object::absolute_position();
        } else {
            return (kmVec3) body()->position();
        }
    }

    //FIXME: Override more!

private:
    MeshPtr mesh_;
    std::vector<std::shared_ptr<SubActor> > subactors_;

    RenderPriority render_priority_;

    sigc::signal<void, ActorID> signal_mesh_changed_;

    void do_update(double dt) {
        update_source(dt);
    }

    std::shared_ptr<PhysicsBody> body_;

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

    const kmVec3 centre() const {
        // Return the centre point of the absolute bounds of this subactor
        // which is the submesh().bounds() transformed by the parent actor's
        // location
        kmVec3 centre;
        kmAABB abs_bounds = absolute_bounds();
        kmAABBCentre(&abs_bounds, &centre);
        return centre;
    }

private:
    Actor& parent_;
    SubMeshIndex index_;
    MaterialPtr material_;

    const SubMesh& submesh() const {
        return parent_.mesh().lock()->submesh(index_);
    }
};

}

#endif // ENTITY_H
