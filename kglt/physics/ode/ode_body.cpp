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
    dMassSetSphereTotal(&mass_, 1.0, 1.0);
    dBodySetMass(body_, &mass_);

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


void ODEBody::set_mass(float mass) {
    dMassSetSphereTotal(&mass_, mass, 1.0);
    dBodySetMass(body_, &mass_);
}

float ODEBody::mass() const {
    return mass_.mass;
}

//FIXME: Don't know if these belong here, depends if physics engines
// support limiting things like this...
void ODEBody::set_max_speed(float speed_limit) {

}

float ODEBody::max_speed() const {

}

void ODEBody::set_min_speed(float speed_limit) {

}

float ODEBody::min_speed() const {

}
//ENDFIXME

void ODEBody::apply_linear_force_global(const kglt::Vec3& force) {

}

void ODEBody::apply_linear_force_local(const kglt::Vec3& force) {

}

void ODEBody::apply_angular_force_global(const kglt::Vec3& force) {}
void ODEBody::apply_angular_force_local(const kglt::Vec3& force) {}

void ODEBody::set_angular_damping(const float amount) {}

void ODEBody::set_angular_velocity(const kglt::Vec3& velocity) {

}

kglt::Vec3 ODEBody::angular_velocity() const {

}

void ODEBody::set_linear_velocity(const kglt::Vec3& velocity) {

}

kglt::Vec3 ODEBody::linear_velocity() const {

}

ConstraintID ODEBody::create_fixed_constaint(PhysicsBody& other) {
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

}
}
