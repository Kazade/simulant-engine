#include "mesh_instancer.h"
#include "../stage.h"

namespace smlt {

uint32_t MeshInstancer::id_counter_ = 0;

MeshInstancer::MeshInstancer(Stage *stage, SoundDriver *sound_driver, MeshPtr mesh):
    TypedDestroyableObject<MeshInstancer, Stage>(stage),
    StageNode(stage, STAGE_NODE_TYPE_MESH_INSTANCER),
    AudioSource(stage, this, sound_driver) {

    set_mesh(mesh);
}

MeshInstancer::~MeshInstancer() {

}

const AABB &MeshInstancer::aabb() const {
    return aabb_;
}

void MeshInstancer::set_mesh(MeshPtr mesh) {
    mesh_ = mesh;
    recalc_aabb();
}

MeshPtr MeshInstancer::mesh() const {
    return mesh_;
}

MeshInstanceID MeshInstancer::new_mesh_instance(const Vec3 &position, const Quaternion &rotation) {
    MeshInstance i;
    i.id = ++MeshInstancer::id_counter_;
    i.transformation = Mat4::from_pos_rot_scale(position, rotation, Vec3());
    instances_.insert(std::make_pair(i.id, i));

    recalc_aabb();

    return i.id;
}

bool MeshInstancer::destroy_mesh_instance(MeshInstanceID mid) {
    auto it = instances_.find(mid);
    if(it != instances_.end()) {
        instances_.erase(it);
        recalc_aabb();

        return true;
    }

    return false;
}

bool MeshInstancer::show_mesh_instance(MeshInstanceID mid) {
    auto it = instances_.find(mid);
    if(it != instances_.end()) {
        it->second.visible = true;
        return true;
    }

    return false;
}

bool MeshInstancer::hide_mesh_instance(MeshInstanceID mid) {
    auto it = instances_.find(mid);
    if(it != instances_.end()) {
        it->second.visible = false;
        return true;
    }

    return false;
}

void MeshInstancer::recalc_aabb() {

}

}

