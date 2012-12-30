#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"

#include "boundable.h"
#include "object.h"
#include "mesh.h"

namespace kglt {

class SubEntity;

class Entity :
    public MeshInterface,
    public Managed<Entity>,
    public generic::Identifiable<EntityID>,
    public Object {

public:
    Entity(SubScene* subscene, EntityID id):
        generic::Identifiable<EntityID>(id),
        Object(subscene),
        mesh_(0) {}

    Entity(SubScene* subscene, EntityID id, MeshID mesh):
        generic::Identifiable<EntityID>(id),
        Object(subscene),
        mesh_(mesh) {
    }

    MeshID mesh() const { return mesh_; }
    bool has_mesh() const { return mesh_ != MeshID(0); }

    void set_mesh(MeshID mesh);

    const VertexData& shared_data() const;

    const uint16_t subentity_count() const {
        return subentities_.size();
    }

    SubEntity& subentity(uint16_t idx) {
        return *subentities_.at(idx);
    }

    const std::vector<std::tr1::shared_ptr<SubEntity> >& _subentities() { return subentities_; }

    sigc::signal<void, EntityID>& signal_mesh_changed() { return signal_mesh_changed_; }

private:
    MeshID mesh_;

    std::vector<std::tr1::shared_ptr<SubEntity> > subentities_;

    friend class SubEntity;

    Mesh& _mesh_ref();

    sigc::signal<void, EntityID> signal_mesh_changed_;
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

    const MaterialID material() const {
        if(material_) {
            return material_;
        }

        return submesh().material();
    }
    void override_material(MaterialID material) { material_ = material; }

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

    const SubMesh& submesh() const { return parent_._mesh_ref().submesh(index_); }
};

}

#endif // ENTITY_H
