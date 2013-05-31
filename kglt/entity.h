#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"

#include "boundable.h"
#include "object.h"
#include "mesh.h"
#include "sound.h"

namespace kglt {

class SubEntity;

class Entity :
    public MeshInterface,
    public Managed<Entity>,
    public generic::Identifiable<EntityID>,
    public Object,
    public Source {

public:
    Entity(Stage* stage, EntityID id);
    Entity(Stage* stage, EntityID id, MeshID mesh);

    MeshID mesh_id() const { return (mesh_) ? mesh_->id() : MeshID(0); }
    MeshRef mesh() const { return mesh_; }
    bool has_mesh() const { return bool(mesh_); }
    void set_mesh(MeshID mesh);

    const VertexData& shared_data() const;

    const uint16_t subentity_count() const {
        return subentities_.size();
    }

    SubEntity& subentity(uint16_t idx) {
        return *subentities_.at(idx);
    }

    const std::vector<std::shared_ptr<SubEntity> >& _subentities() { return subentities_; }

    sigc::signal<void, EntityID>& signal_mesh_changed() { return signal_mesh_changed_; }

    void destroy();

    RenderPriority render_priority() const { return render_priority_; }
    void set_render_priority(RenderPriority value) { render_priority_ = value;}
private:
    MeshPtr mesh_;
    std::vector<std::shared_ptr<SubEntity> > subentities_;

    RenderPriority render_priority_;

    sigc::signal<void, EntityID> signal_mesh_changed_;

    void do_update(double dt) {
        update_source(dt);
    }

    friend class SubEntity;
};

class SubEntity :
    public SubMeshInterface,
    public Managed<SubEntity>,
    public Boundable {

public:
    SubEntity(Entity& parent, SubMeshIndex idx):
        parent_(parent),
        index_(idx),
        material_(0) {
    }

    const MaterialID material_id() const {
        if(material_) {
            return material_;
        }

        return submesh().material_id();
    }

    const SubMeshIndex submesh_id() const { return index_; }

    void override_material_id(MaterialID material) { material_ = material; }

    const VertexData& vertex_data() const { return submesh().vertex_data(); }
    const IndexData& index_data() const { return submesh().index_data(); }
    const MeshArrangement arrangement() const { return submesh().arrangement(); }

    Entity& _parent() { return parent_; }

    /* Boundable interface implementation */

    /**
     * @brief absolute_bounds
     * @return the bounds of the linked submesh, transformed by the entities absolute
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
        // Return the centre point of the absolute bounds of this subentity
        // which is the submesh().bounds() transformed by the parent entity's
        // location
        kmVec3 centre;
        kmAABB abs_bounds = absolute_bounds();
        kmAABBCentre(&abs_bounds, &centre);
        return centre;
    }

private:
    Entity& parent_;
    SubMeshIndex index_;
    MaterialID material_;

    const SubMesh& submesh() const {
        return parent_.mesh().lock()->submesh(index_);
    }
};

}

#endif // ENTITY_H
