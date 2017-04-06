#pragma once

#include "collider.h"

#include "../controller.h"
#include "../../generic/property.h"
#include "../../types.h"

struct b3World;
struct b3Body;
struct b3Hull;


namespace smlt {

class StageNode;

namespace controllers {

class RigidBodySimulation;

namespace impl {

class Body:
    public Controller {

public:
    Body(Controllable* object, RigidBodySimulation* simulation, GeneratedColliderType collider=GENERATED_COLLIDER_TYPE_NONE);
    virtual ~Body();

    void move_to(const Vec3& position);
    void rotate_to(const Quaternion& rotation);

    bool init();
    void cleanup();

    void add_box_collider(
        const Vec3& size,
        const ColliderMaterial& properties,
        const Vec3& offset, const Quaternion& rotation
    );

    void add_sphere_collider(
        const float radius,
        const ColliderMaterial& properties,
        const Vec3& offset, const Quaternion& rotation
    );

    void add_mesh_collider(
        const MeshID& mesh,
        const ColliderMaterial& properties,
        const Vec3& offset, const Quaternion& rotation
    );

    Property<Body, RigidBodySimulation> simulation = {
        this, [](Body* _this) -> RigidBodySimulation* {
            if(auto ret = _this->simulation_.lock()) {
                return ret.get();
            } else {
                return nullptr;
            }
        }
    };

protected:
    friend class smlt::controllers::RigidBodySimulation;
    StageNode* object_;
    b3Body* body_ = nullptr;
    std::weak_ptr<RigidBodySimulation> simulation_;
    GeneratedColliderType collider_type_;

    std::pair<Vec3, Quaternion> last_state_;

    void update(float dt) override;

    void build_collider(GeneratedColliderType collider);

private:
    virtual bool is_dynamic() const { return true; }

    sig::connection simulation_stepped_connection_;
    std::vector<std::shared_ptr<b3Hull>> hulls_;
};

} // End impl
}
}
