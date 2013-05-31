#include "subscene.h"
#include "entity.h"

namespace kglt {

Entity::Entity(Stage* subscene, EntityID id):
    generic::Identifiable<EntityID>(id),
    Object(subscene),
    Source(*subscene),
    render_priority_(RENDER_PRIORITY_MAIN) {

}

Entity::Entity(Stage* subscene, EntityID id, MeshID mesh):
    generic::Identifiable<EntityID>(id),
    Object(subscene),
    Source(*subscene),
    render_priority_(RENDER_PRIORITY_MAIN) {

    set_mesh(mesh);
}

const VertexData& Entity::shared_data() const {
    return mesh_->shared_data();
}

void Entity::set_mesh(MeshID mesh) {
    //Increment the ref-count on this mesh
    mesh_ = subscene().mesh(mesh).lock();

    subentities_.clear();
    for(SubMeshIndex idx: mesh_->submesh_ids()) {
        subentities_.push_back(SubEntity::create(*this, idx));
    }

    signal_mesh_changed_(id());
}

void Entity::destroy() {
    subscene().delete_entity(id());
}

}
