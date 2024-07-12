#include "static_body.h"
#include "../services/physics.h"
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

    return PhysicsBody::on_create(params);
}
} // namespace smlt
