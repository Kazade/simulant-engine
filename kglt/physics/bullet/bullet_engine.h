#ifndef BULLET_ENGINE_H
#define BULLET_ENGINE_H

#include <memory>
#include <unordered_map>

#include "../types.h"
#include "../../generic/managed.h"
#include "../physics_engine.h"

#include <btBulletDynamicsCommon.h>

namespace kglt {
namespace physics {

class BulletBody;
class BulletCollidable;

class BulletEngine :
    public PhysicsEngine,
    public Managed<BulletEngine> {

public:
    using PhysicsEngine::init;
    using PhysicsEngine::cleanup;

private:
    bool do_init();
    void do_cleanup();
    void do_update(double dt);
    void do_step(double dt);
    //Factory function
    std::shared_ptr<ResponsiveBody> do_new_responsive_body(kglt::Object *owner);
    std::shared_ptr<Collidable> do_new_collidable(kglt::Object *owner);

    ShapeID do_create_plane(float a, float b, float c, float d);
    void do_set_gravity(const Vec3& gravity);

    friend class BulletBody;
    friend class BulletCollidable;

    std::unordered_map<ShapeID, std::shared_ptr<Collidable> > shapes_;

    std::unique_ptr<btBroadphaseInterface> broadphase_;
    std::unique_ptr<btDefaultCollisionConfiguration> collision_configuration_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
    std::unique_ptr<btDiscreteDynamicsWorld> world_;

    void on_scene_set(Scene &scene) override;

    struct KinematicObject {
        btRigidBody* body;
        btCollisionShape* shape;
        btDefaultMotionState* motion_state;

        KinematicObject():
            body(nullptr), shape(nullptr), motion_state(nullptr) {}

        ~KinematicObject() {
            delete motion_state;
            delete shape;
            delete body;
        }
    };

    std::unordered_map<ShapeID, std::shared_ptr<KinematicObject>> static_objects_;
};

}
}

#endif // BULLET_ENGINE_H
