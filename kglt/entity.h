#ifndef ENTITY_H
#define ENTITY_H

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/relation.h"

#include "object.h"
#include "mesh.h"

namespace kglt {

class SubEntity;

class Entity :
    public MeshInterface,
    public Managed<Entity>,
    public generic::Identifiable<EntityID>,
    public Object,
    public Relatable {

public:
    Relation<Entity, SceneGroup> scene_group;

    Entity(Scene* scene, EntityID id):
        generic::Identifiable<EntityID>(id),
        Object(scene),
        scene_group(this),
        mesh_(0) {}

    Entity(Scene* scene, EntityID id, MeshID mesh):
        generic::Identifiable<EntityID>(id),
        Object(scene),
        scene_group(this),
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
private:
    MeshID mesh_;

    std::vector<std::tr1::shared_ptr<SubEntity> > subentities_;

    friend class SubEntity;

    Mesh& _mesh_ref();
};

class SubEntity :
    public SubMeshInterface,
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
    const MeshArrangement arrangement() const { return submesh().arrangement(); }

    Entity& _parent() { return parent_; }
private:
    Entity& parent_;
    uint16_t index_;
    MaterialID material_;

    const SubMesh& submesh() const { return parent_._mesh_ref().submesh(index_); }
};

}

#endif // ENTITY_H
