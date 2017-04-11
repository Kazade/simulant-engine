#pragma once

#include <unordered_map>
#include <utility>
#include <set>

#include "collider.h"

#include "../controller.h"
#include "../../generic/property.h"
#include "../../types.h"

struct b3World;
struct b3Body;
struct b3Hull;
struct b3Mesh;
struct b3Triangle;
struct b3Vec3;

namespace smlt {

namespace utils {
    struct Triangle;
}

class StageNode;

namespace controllers {

class RigidBodySimulation;
class CollisionListener;

namespace impl {

class Body:
    public Controller {

public:
    Body(Controllable* object, RigidBodySimulation* simulation);
    virtual ~Body();

    void move_to(const Vec3& position);
    void rotate_to(const Quaternion& rotation);

    bool init();
    void cleanup();

    void add_box_collider(
        const Vec3& size,
        const PhysicsMaterial& properties,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );

    void add_sphere_collider(const float diameter,
        const PhysicsMaterial& properties,
        const Vec3& offset=Vec3()
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

    void register_collision_listener(CollisionListener* listener);

    void unregister_collision_listener(CollisionListener* listener);

protected:
    friend class smlt::controllers::RigidBodySimulation;
    StageNode* object_;
    b3Body* body_ = nullptr;
    std::weak_ptr<RigidBodySimulation> simulation_;

    std::pair<Vec3, Quaternion> last_state_;

    void update(float dt) override;

private:
    virtual bool is_dynamic() const { return true; }

    sig::connection simulation_stepped_connection_;
    std::vector<std::shared_ptr<b3Hull>> hulls_;
    std::set<CollisionListener*> listeners_;
};

} // End impl
}
}
