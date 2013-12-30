#include <cstdint>

#include "bullet_body.h"
#include "bullet_engine.h"

namespace kglt {
namespace physics {

static int32_t constraint_counter = 1;

bool BulletBody::do_init() {
    compound_shape_ = std::make_unique<btCompoundShape>();

    motion_state_ = std::make_unique<btDefaultMotionState>(
        btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0))
    );

    btRigidBody::btRigidBodyConstructionInfo c_info(
        1, motion_state_.get(), compound_shape_.get(),btVector3(0,0,0)
    );

    body_ = std::make_unique<btRigidBody>(c_info);

    BulletEngine& eng = dynamic_cast<BulletEngine&>(*engine());
    eng.world_->addRigidBody(body_.get());

    body_->setActivationState(DISABLE_DEACTIVATION);

    //Initialize spherical mass of 1.0
    set_mass_sphere(1.0, 1.0);

    return true;
}

void BulletBody::do_cleanup() {

}

void BulletBody::do_set_position(const kglt::Vec3& position) {
    //FIXME: Should deactivate, move, then reactivate
    auto orig_flags = body_->getCollisionFlags();
    body_->setCollisionFlags(orig_flags | btCollisionObject::CF_KINEMATIC_OBJECT);

    btTransform transform = body_->getCenterOfMassTransform();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    body_->setCenterOfMassTransform(transform);
    body_->clearForces();
    body_->setCollisionFlags(orig_flags);
}

kglt::Vec3 BulletBody::do_position() const {
    btVector3 pos = body_->getWorldTransform().getOrigin();
    return kglt::Vec3(pos.x(), pos.y(), pos.z());
}

void BulletBody::do_set_rotation(const kglt::Quaternion& quat) {
    //FIXME: Should deactivate, move, then reactivate
    //body->setCollisionFlags(body->getCollisionFlags()|btCollisionObject::CF_KINEMATIC_OBJECT);
    //body->setActivationState(DISABLE_DEACTIVATION);

    btTransform transform = body_->getCenterOfMassTransform();
    transform.setRotation(btQuaternion(quat.x, quat.y, quat.z, quat.w));
//    body_->setCenterOfMassTransform(transform);
}

kglt::Quaternion BulletBody::do_rotation() const {
    btQuaternion rot = body_->getWorldTransform().getRotation();
    return Quaternion(rot.x(), rot.y(), rot.z(), rot.w());
}

void BulletBody::do_apply_linear_force_global(const kglt::Vec3& force) {
    btVector3 linear(force.x, force.y, force.z);
    body_->applyCentralForce(linear);
}

void BulletBody::do_apply_linear_force_local(const kglt::Vec3& force) {
    //Bullet only takes forces in world space, so we need to transform it
    btTransform t;
    body_->getMotionState()->getWorldTransform(t);
    t.setOrigin(btVector3(0,0,0));

    btVector3 local_force(force.x, force.y, force.z);
    body_->applyCentralForce(t * local_force);
}

void BulletBody::do_apply_angular_force_global(const kglt::Vec3& force) {
    btVector3 torque(force.x, force.y, force.z);
    body_->applyTorque(torque);
}

void BulletBody::do_apply_angular_force_local(const kglt::Vec3& force) {
    btVector3 torque(force.x, force.y, force.z);

    //Transform the torque to local space
    torque = body_->getInvInertiaTensorWorld().inverse() * (body_->getWorldTransform().getBasis() * torque);

    body_->applyTorque(torque);
}

void BulletBody::do_set_angular_damping(const float amount) {
    body_->setDamping(body_->getLinearDamping(), amount);
}

void BulletBody::do_set_linear_damping(const float amount) {
    body_->setDamping(amount, body_->getAngularDamping());
}

void BulletBody::do_set_angular_velocity(const kglt::Vec3& velocity) {
    body_->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
}

kglt::Vec3 BulletBody::do_angular_velocity() const {
    btVector3 vel = body_->getAngularVelocity();
    return kglt::Vec3(vel.x(), vel.y(), vel.z());
}

void BulletBody::do_set_linear_velocity(const kglt::Vec3& vel) {
    body_->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
}

kglt::Vec3 BulletBody::do_linear_velocity() const {
    btVector3 vel = body_->getLinearVelocity();
    return kglt::Vec3(vel.x(), vel.y(), vel.z());
}

ConstraintID BulletBody::do_create_fixed_constraint(ResponsiveBody &other) {
    L_WARN("Constraints not implemented in Bullet backend");

    ConstraintID new_id(constraint_counter++);

    /*ODEEngine& eng = dynamic_cast<ODEEngine&>(*engine());

    dJointID new_joint = dJointCreateFixed(eng.world(), 0);
    dJointAttach(new_joint, body_, dynamic_cast<BulletBody&>(other).body_);
    dJointSetFixed(new_joint);



    constraints_[new_id] = new_joint;

    enable_constraint(new_id);*/

    return new_id;
}

void BulletBody::do_destroy_constraint(ConstraintID c) {
    L_WARN("Constraints not implemented in Bullet backend");
    //dJointDestroy(constraints_.at(c));
    //constraints_.erase(c);
}

void BulletBody::do_enable_constraint(ConstraintID c) {
    L_WARN("Constraints not implemented in Bullet backend");
    //dJointEnable(constraints_.at(c));
}

void BulletBody::do_disable_constraint(ConstraintID c) {
    L_WARN("Constraints not implemented in Bullet backend");
    //dJointDisable(constraints_.at(c));
}

void BulletBody::do_set_mass(float total_mass, kglt::Vec3 inertia) {
    btScalar m = total_mass;
    btVector3 i(inertia.x, inertia.y, inertia.z);
    if(!i.length()) {
        compound_shape_->calculateLocalInertia(m, i);
    }
    body_->setMassProps(m, i);
}

void BulletBody::do_set_mass_sphere(float total_mass, float radius) {
    //FIXME: Calculate inertia correctly
    L_WARN("Spherical inertia is not yet calculated, using AABB");

    btScalar m = total_mass;
    btVector3 i;
    compound_shape_->calculateLocalInertia(m, i);

    BulletEngine& eng = dynamic_cast<BulletEngine&>(*engine());
    eng.world_->removeRigidBody(body_.get());
    body_->setMassProps(m, i);
    eng.world_->addRigidBody(body_.get());
}

void BulletBody::do_set_mass_box(float total_mass, float width, float height, float depth) {
    //FIXME: Calculate inertia correctly
    L_WARN("User-defined box inertia is not yet calculated, using AABB");

    btScalar m = total_mass;
    btVector3 i;
    compound_shape_->calculateLocalInertia(m, i);

    BulletEngine& eng = dynamic_cast<BulletEngine&>(*engine());

    eng.world_->removeRigidBody(body_.get());
    body_->setCollisionFlags(body_->getCollisionFlags() |btCollisionObject::CF_KINEMATIC_OBJECT);
    body_->setMassProps(m, i);
    eng.world_->addRigidBody(body_.get());
}

}
}
