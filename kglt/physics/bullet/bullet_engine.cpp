#include "../../types.h"
#include "../../scene.h"
#include "../../window_base.h"

#include "bullet_body.h"
#include "bullet_engine.h"
#include "bullet_collidable.h"

namespace kglt {
namespace physics {

bool BulletEngine::do_init() {
    broadphase_ = std::make_unique<btDbvtBroadphase>();
    collision_configuration_ = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher_ = std::make_unique<btCollisionDispatcher>(collision_configuration_.get());
    solver_ = std::make_unique<btSequentialImpulseConstraintSolver>();
    world_ = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher_.get(),
        broadphase_.get(),
        solver_.get(),
        collision_configuration_.get()
    );

    return true;
}

void BulletEngine::do_cleanup() {
    broadphase_.reset();
    collision_configuration_.reset();
    dispatcher_.reset();
    solver_.reset();
    world_.reset();
}

void BulletEngine::on_scene_set(Scene &scene) {
    //Connect to the update signal, rather than using the do_step function to update the world
    scene.window().signal_update().connect(std::bind(&BulletEngine::do_update, this, std::placeholders::_1));
}

void BulletEngine::do_update(double dt) {
    world_->stepSimulation(dt, 10);
}

void BulletEngine::do_step(double dt) {
    //Do nothing here. Bullet has its own time accumulator so we need to connect
    //to the update signal, not step
}

ShapeID BulletEngine::do_create_plane(float a, float b, float c, float d) {
    /*
     *  Create a KinematicObject representing the plane
     */
    auto obj = std::make_shared<KinematicObject>();
    obj->motion_state = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)));
    obj->shape = new btStaticPlaneShape(btVector3(a, b, c), d);

    btRigidBody::btRigidBodyConstructionInfo c_info(
        0, obj->motion_state, obj->shape, btVector3(0,0,0)
    );

    obj->body = new btRigidBody(c_info);

    world_->addRigidBody(obj->body);

    obj->body->forceActivationState(DISABLE_DEACTIVATION);
    obj->body->activate();

    ShapeID new_id = get_next_shape_id();

    static_objects_.insert(std::make_pair(new_id, obj));

    return new_id;
}

void BulletEngine::do_set_gravity(const Vec3& gravity) {
    world_->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}

std::shared_ptr<ResponsiveBody> BulletEngine::do_new_responsive_body(Object* owner) {
    auto result = std::make_shared<BulletBody>(owner);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}

std::shared_ptr<Collidable> BulletEngine::do_new_collidable(Object* owner) {
    auto result = std::make_shared<BulletCollidable>(owner, this);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}

}
}
