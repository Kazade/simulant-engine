#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "newmesh.h"

namespace kglt {

class SubEntity;

class Entity :
    public newmesh::MeshInterface,
    public Managed<Entity>,
    public generic::Identifiable<EntityID> {

public:
    Entity(Scene* scene, EntityID id):
        generic::Identifiable<EntityID>(id),
        scene_(*scene),
        mesh_(0) {}

    Entity(Scene* scene, EntityID id, newmesh::MeshID mesh):
        generic::Identifiable<EntityID>(id),
        scene_(*scene),
        mesh_(mesh) {
    }

    newmesh::MeshID mesh() const { return mesh_; }
    bool has_mesh() const { return mesh_ != newmesh::MeshID(0); }

    void set_mesh(newmesh::MeshID mesh);

    const VertexData& shared_data() const;

    const uint16_t subentity_count() const {
        return subentities_.size();
    }

    SubEntity& subentity(uint16_t idx) {
        return *subentities_.at(idx);
    }

private:
    Scene& scene_;
    newmesh::MeshID mesh_;

    std::vector<std::tr1::shared_ptr<SubEntity> > subentities_;

    friend class SubEntity;

    newmesh::Mesh& _mesh_ref();
};

class SubEntity :
    public newmesh::SubMeshInterface,
    public Managed<SubEntity> {

public:
    SubEntity(Entity& parent, uint16_t idx):
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

private:
    Entity& parent_;
    uint16_t index_;
    MaterialID material_;

    const newmesh::SubMesh& submesh() const { return parent_._mesh_ref().submesh(index_); }
};

}

#endif // ENTITY_H
