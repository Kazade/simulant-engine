#include "rigid_body.h"
#include "../object.h"
#include "../actor.h"
#include "../stage.h"

namespace kglt {
namespace controllers {

q3Vec3 to_q3vec3(const Vec3& rhs) {
    q3Vec3 ret;
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
    return ret;
}

Mat3 from_q3mat3(const q3Mat3& rhs) {
    Mat3 ret;
    ret.mat[0] = rhs.Column0().x;
    ret.mat[3] = rhs.Column0().y;
    ret.mat[6] = rhs.Column0().z;

    ret.mat[1] = rhs.Column1().x;
    ret.mat[4] = rhs.Column1().y;
    ret.mat[7] = rhs.Column1().z;

    ret.mat[2] = rhs.Column2().x;
    ret.mat[5] = rhs.Column2().y;
    ret.mat[8] = rhs.Column2().z;

    return ret;
}

RigidBodySimulation::RigidBodySimulation() {
    scene_ = new q3Scene(1.0 / 60.0f);
    scene_->SetAllowSleep(true);
    scene_->SetGravity(q3Vec3(0, -9.81, 0));
}

bool RigidBodySimulation::init() {

    return true;
}

void RigidBodySimulation::cleanup() {

}



void RigidBodySimulation::step(double dt) {
    scene_->Step();
}

Vec3 RigidBodySimulation::intersect_ray(const Vec3& start, const Vec3& direction) {

}

q3Body* RigidBodySimulation::acquire_body(impl::Body *body) {
    q3BodyDef def;

    bool is_dynamic = body->is_dynamic();
    def.bodyType = (is_dynamic) ? eDynamicBody : eStaticBody;
    def.gravityScale = (is_dynamic) ? 1.0 : 0.0;

    bodies_[body] = scene_->CreateBody(def);
    return bodies_[body];
}

void RigidBodySimulation::release_body(impl::Body *body) {
    scene_->RemoveBody(bodies_.at(body));
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(impl::Body *body) {
    q3Body* b = bodies_.at(body);
    auto xform = b->GetTransform();
    Mat3 rot = from_q3mat3(xform.rotation);

    return std::make_pair(
        Vec3(xform.position.x, xform.position.y, xform.position.z),
        Quaternion(rot)
    );
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    q3Body* b = bodies_.at(body);

    Vec3 axis;
    float angle;
    kmQuaternionToAxisAngle(&rotation, &axis, &angle);

    b->SetTransform(to_q3vec3(position), to_q3vec3(axis), angle);
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Body(object, simulation) {

}

RigidBody::~RigidBody() {

}

void RigidBody::add_force(const Vec3 &force) {
    q3Body* b = simulation_->bodies_.at(this);
    b->ApplyLinearForce(to_q3vec3(force));
}

void RigidBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    q3Body* b = simulation_->bodies_.at(this);

    q3Vec3 f, p;

    f.x = force.x;
    f.y = force.y;
    f.z = force.z;

    p.x = position.x;
    p.y = position.y;
    p.z = position.z;

    b->ApplyForceAtWorldPoint(to_q3vec3(force), to_q3vec3(position));
}

void RigidBody::add_torque(const Vec3& torque) {
    q3Body* b = simulation_->bodies_.at(this);
    b->ApplyTorque(to_q3vec3(torque));
}

StaticBody::StaticBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Body(object, simulation) {

}

StaticBody::~StaticBody() {

}


namespace impl {

Body::Body(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider_type):
    Controller("rigid-body"),
    simulation_(simulation),
    collider_type_(collider_type) {

    object_ = dynamic_cast<MoveableObject*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }


}

Body::~Body() {

}

bool Body::init() {
    body_ = simulation_->acquire_body(this);
    build_collider(collider_type_);

    return true;
}

void Body::cleanup() {
    simulation_->release_body(this);
}

void Body::move_to(const Vec3& position) {
    auto xform = simulation_->body_transform(this);
    simulation_->set_body_transform(
        this,
        position,
        xform.second
    );
}

void Body::do_post_update(double dt) {
    auto xform = simulation_->body_transform(this);
    object_->set_absolute_position(xform.first);
    object_->set_absolute_rotation(xform.second);
}

void Body::build_collider(ColliderType collider) {
    if(collider == COLLIDER_TYPE_NONE) return;

    if(collider == COLLIDER_TYPE_BOX) {
        BoundableEntity* entity = dynamic_cast<BoundableEntity*>(object_);
        if(entity) {
            AABB aabb = entity->aabb();

            q3BoxDef def;
            def.SetRestitution(0);

            q3Transform localSpace;
            q3Identity( localSpace );

            def.Set(localSpace, q3Vec3(aabb.width(), aabb.height(), aabb.depth()));
            simulation_->bodies_.at(this)->AddBox(def);
        }
    }
}

}



}
}
