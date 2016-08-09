#include "rigid_body.h"
#include "../object.h"

namespace kglt {
namespace controllers {

RigidBodySimulation::RigidBodySimulation(uint32_t max_bodies):
    max_bodies_(max_bodies) {

}

bool RigidBodySimulation::init() {
    scene_ = ysr_init_scene(max_bodies_);

    // Disable all bodies, only enable them as we add them
    uint32_t i = 0;
    for(auto& body: scene_->bodies) {
        body.mass = 0;
        free_bodies_.push(i++);
    }

    return true;
}

void RigidBodySimulation::cleanup() {
    ysr_free_scene(scene_);
}

Mat3 from_ym_mat3f(const ym_mat3f& rhs) {
    Mat3 ret;
    for(uint32_t i = 0; i < 9; ++i) {
        ret.mat[i] = rhs.data()[i];
    }
    return ret;
}

void RigidBodySimulation::step(double dt) {

    // Apply force and torque:
    for(auto& p: bodies_) {
        RigidBody* body = dynamic_cast<RigidBody*>(p.first);
        if(!body) {
            continue;
        }

        uint32_t i = p.second;

        float inv_mass = scene_->bodies[i].mass_inv;
        Vec3 additional_linear = body->force_ * (inv_mass * dt);

        scene_->lin_vel[i].v[0] += additional_linear.x;
        scene_->lin_vel[i].v[1] += additional_linear.y;
        scene_->lin_vel[i].v[2] += additional_linear.z;

        Mat3 inv_inertia = from_ym_mat3f(scene_->bodies[i].inertia_inv);

        Vec3 additional_angular;
        kmVec3MultiplyMat3(&additional_angular, &body->torque_, &inv_inertia);
        additional_angular *= dt;

        scene_->ang_vel[i].v[0] += additional_angular.x;
        scene_->ang_vel[i].v[1] += additional_angular.y;
        scene_->ang_vel[i].v[2] += additional_angular.z;

        body->force_ = Vec3();
        body->torque_ = Vec3();
    }

    ysr_advance(scene_, dt);
}

uint32_t RigidBodySimulation::acquire_body(impl::Body *body) {
    if(free_bodies_.empty()) {
        throw std::runtime_error("Hit the maximum number of bodies");
    }

    uint32_t i = free_bodies_.front();
    free_bodies_.pop();
    bodies_[body] = i;
    scene_->bodies[i].mass = 1.0; // Enable the body in the simulation   
    return i;
}

void RigidBodySimulation::release_body(impl::Body *body) {
    auto i = bodies_.at(body);
    scene_->bodies[i].mass = 0;
    bodies_.erase(body);
    free_bodies_.push(i);
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(impl::Body *body) {
    ym_frame3f frame = ysr_get_transform(scene_, bodies_.at(body));

    return std::make_pair(
        Vec3(frame.t[0], frame.t[1], frame.t[2]),
        Quaternion(Mat3(frame.m.data()))
    );
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    ym_frame3f frame;
    frame.t[0] = position.x;
    frame.t[1] = position.y;
    frame.t[2] = position.z;

    Mat3 rot;
    kmMat3FromRotationQuaternion(&rot, &rotation);
    for(uint32_t i = 0; i < 9; ++i) { frame.m.data()[i] = rot.mat[i]; }

    ysr_set_transform(scene_, bodies_.at(body), frame);
}

ysr__body* RigidBodySimulation::get_ysr_body(impl::Body* body) {
    return &scene_->bodies[bodies_.at(body)];
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Body(object, simulation) {

}

RigidBody::~RigidBody() {

}

void RigidBody::add_force(const Vec3 &force) {
    force_ += force;
}

void RigidBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    // FIXME: Should use shape position, probably...
    Vec3 com = simulation_->body_transform(this).first;

    Vec3 centre_dir = position - com;
    Vec3 torque_force = centre_dir.cross(force);

    add_force(force);
    add_torque(torque_force);
}

void RigidBody::add_torque(const Vec3& torque) {
    torque_ += torque;
}

StaticBody::StaticBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Body(object, simulation) {

    // Make sure we set the mass to zero so we don't simulate this body
    auto body = simulation->get_ysr_body(this);
    body->mass = 0;
}

StaticBody::~StaticBody() {

}


namespace impl {

Body::Body(Controllable* object, RigidBodySimulation::ptr simulation):
    Controller("rigid-body"),
    simulation_(simulation) {

    object_ = dynamic_cast<MoveableObject*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }

    body_id_ = simulation->acquire_body(this);
}

Body::~Body() {
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

}



}
}
