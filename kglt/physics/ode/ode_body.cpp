#include "../../actor.h"
#include "../../stage.h"
#include "../../scene.h"
#include "../../object.h"

#include "ode_body.h"
#include "ode_engine.h"

namespace kglt {
namespace physics {

static int32_t constraint_counter = 1;

bool ODEBody::init() {
    ODEEngine* engine = dynamic_cast<ODEEngine*>(owner()->stage().scene().physics_engine());

    body_ = dBodyCreate(engine->world());

    //Initialize spherical mass of 1.0
    set_mass_sphere(1.0, 1.0);

    return true;
}

void ODEBody::cleanup() {
    dBodyDestroy(body_);
}

void ODEBody::set_position(const kglt::Vec3& position) {
    dBodySetPosition(body_, position.x, position.y, position.z);
}

kglt::Vec3 ODEBody::position() const {
    const dReal* pos = dBodyGetPosition(body_);
    return kglt::Vec3(pos[0], pos[1], pos[2]);
}

void ODEBody::set_rotation(const kglt::Quaternion& quat) {
    dQuaternion q;
    q[0] = quat.w;
    q[1] = quat.x;
    q[2] = quat.y;
    q[3] = quat.z;
    dBodySetQuaternion(body_, q);
}

kglt::Quaternion ODEBody::rotation() const {
    const dReal* rot = dBodyGetQuaternion(body_);
    return Quaternion(rot[1], rot[2], rot[3], rot[0]);
}

void ODEBody::apply_linear_force_global(const kglt::Vec3& force) {
    dBodyAddForce(body_, force.x, force.y, force.z);
}

void ODEBody::apply_linear_force_local(const kglt::Vec3& force) {
    dBodyAddRelForce(body_, force.x, force.y, force.z);
}

void ODEBody::apply_angular_force_global(const kglt::Vec3& force) {
    dBodyAddTorque(body_, force.x, force.y, force.z);
}

void ODEBody::apply_angular_force_local(const kglt::Vec3& force) {
    dBodyAddRelTorque(body_, force.x, force.y, force.z);
}

void ODEBody::set_angular_damping(const float amount) {
    dBodySetAngularDamping(body_, amount);
}

void ODEBody::set_angular_velocity(const kglt::Vec3& velocity) {
    dBodySetAngularVel(body_, velocity.x, velocity.y, velocity.z);
}

kglt::Vec3 ODEBody::angular_velocity() const {
    const dReal* pos = dBodyGetAngularVel(body_);
    return kglt::Vec3(pos[0], pos[1], pos[2]);
}

void ODEBody::set_linear_velocity(const kglt::Vec3& vel) {
    dBodySetLinearVel(body_, vel.x, vel.y, vel.z);
}

kglt::Vec3 ODEBody::linear_velocity() const {
    const dReal* pos = dBodyGetLinearVel(body_);
    return kglt::Vec3(pos[0], pos[1], pos[2]);
}

ConstraintID ODEBody::create_fixed_constaint(ResponsiveBody &other) {
    ODEEngine* engine = dynamic_cast<ODEEngine*>(owner()->stage().scene().physics_engine());

    dJointID new_joint = dJointCreateFixed(engine->world(), 0);
    dJointAttach(new_joint, body_, dynamic_cast<ODEBody&>(other).body_);
    dJointSetFixed(new_joint);

    ConstraintID new_id(constraint_counter++);

    constraints_[new_id] = new_joint;

    enable_constraint(new_id);

    return new_id;
}

void ODEBody::destroy_constraint(ConstraintID c) {
    dJointDestroy(constraints_.at(c));
    constraints_.erase(c);
}

void ODEBody::enable_constraint(ConstraintID c) {
    dJointEnable(constraints_.at(c));
}

void ODEBody::disable_constraint(ConstraintID c) {
    dJointDisable(constraints_.at(c));
}

void ODEBody::do_set_mass_sphere(float total_mass, float radius) {
    dMassSetSphereTotal(&mass_, total_mass, radius);
    dBodySetMass(body_, &mass_);
}

void ODEBody::do_set_mass_box(float total_mass, float width, float height, float depth) {
    dMassSetBoxTotal(&mass_, total_mass, width, height, depth);
    dBodySetMass(body_, &mass_);
}

}
}
