#include "static_body.h"
#include "../../services/physics.h"
#include "simulant/utils/params.h"

namespace smlt {

void StaticBody::add_mesh_collider(const MeshPtr& mesh,
                                   const PhysicsMaterial& properties,
                                   uint16_t kind, const Vec3& offset,
                                   const Quaternion& rotation) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    return simulation->add_mesh_collider(this, mesh, properties, kind, offset,
                                         rotation);
}

bool StaticBody::on_create(Params params) {
    if(!clean_params<StaticBody>(params)) {
        return false;
    }

    if(!PhysicsBody::on_create(params)) {
        return false;
    }

    if(params.contains("mesh")) {
        auto mesh = params.get<MeshPtr>("mesh").value_or(MeshPtr());
        if(mesh) {
            auto material = PhysicsMaterial(
                params.get<float>("density").value_or(0.1f),
                params.get<float>("friction").value_or(0.2f),
                params.get<float>("bounciness").value_or(0.00001f));
            add_mesh_collider(mesh, material);
        }
    }

    return true;
}
} // namespace smlt
