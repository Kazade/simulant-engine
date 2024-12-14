#include "static_body.h"
#include "../../services/physics.h"
#include "simulant/utils/params.h"

namespace smlt {

void StaticBody::add_mesh_collider(const MeshPtr& mesh,
                                   const PhysicsMaterial& properties,
                                   uint16_t kind, const Vec3& offset,
                                   const Quaternion& rotation,
                                   const Vec3& scale) {
    auto simulation = get_simulation();
    if(!simulation) {
        return;
    }

    return simulation->add_mesh_collider(this, mesh, properties, kind, offset,
                                         rotation, scale);
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

            auto position = params.get<FloatArray>("position")
                                .value_or(FloatArray{0, 0, 0});
            auto orientation =
                params.get<FloatArray>("orientation").value_or({0, 0, 0, 1});
            auto scale = params.get<FloatArray>("scale").value_or({1, 1, 1});

            // FIXME: The zero here is the "kind". We should:
            // 1. Rename this to "tag" or "group" or "layer"
            // 2. Make it a parameter to the stage node
            // 3. Pass it down!
            add_mesh_collider(mesh, material, 0, position, orientation, scale);
        }
    }

    return true;
}
} // namespace smlt
