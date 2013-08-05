#include "stage.h"
#include "actor.h"

namespace kglt {

Actor::Actor(Stage* stage, ActorID id):
    generic::Identifiable<ActorID>(id),
    ParentSetterMixin<Object>(stage),
    Source(stage),
    render_priority_(RENDER_PRIORITY_MAIN) {

}

Actor::Actor(Stage* stage, ActorID id, MeshID mesh):
    generic::Identifiable<ActorID>(id),
    ParentSetterMixin<Object>(stage),
    Source(stage),
    render_priority_(RENDER_PRIORITY_MAIN) {

    set_mesh(mesh);
}

void Actor::override_material_id(MaterialID mat) {
    for(SubActor::ptr se: subactors_) {
        se->override_material_id(mat);
    }
}

const VertexData& Actor::shared_data() const {
    return mesh_->shared_data();
}

void Actor::set_mesh(MeshID mesh) {
    //Increment the ref-count on this mesh
    mesh_ = stage().mesh(mesh).lock();

    subactors_.clear();
    for(SubMeshIndex idx: mesh_->submesh_ids()) {
        subactors_.push_back(SubActor::create(*this, idx));
    }

    signal_mesh_changed_(id());
}

void Actor::destroy() {
    stage().delete_actor(id());
}

const MaterialID SubActor::material_id() const {
    if(material_) {
        return material_->id();
    }

    return submesh().material_id();
}

void SubActor::override_material_id(MaterialID material) {
    //Store the pointer to maintain the ref-count
    material_ = parent_.stage().material(material).__object;
}

}
