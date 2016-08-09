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

void RigidBodySimulation::step(double dt) {

    // Apply force and torque:
    for(auto& p: bodies_) {
        RigidBody* body = p.first;
        uint32_t i = p.second;

        float inv_mass = scene_->bodies[i].mass_inv;
        Vec3 additional_linear = body->force_ * (inv_mass * dt);

        scene_->lin_vel[i].v[0] += additional_linear.x;
        scene_->lin_vel[i].v[1] += additional_linear.y;
        scene_->lin_vel[i].v[2] += additional_linear.z;
    }

    ysr_advance(scene_, dt);
}

uint32_t RigidBodySimulation::acquire_body(RigidBody* body) {
    if(free_bodies_.empty()) {
        throw std::runtime_error("Hit the maximum number of bodies");
    }

    uint32_t i = free_bodies_.front();
    free_bodies_.pop();
    bodies_[body] = i;
    scene_->bodies[i].mass = 1.0; // Enable the body in the simulation   
    return i;
}

void RigidBodySimulation::release_body(RigidBody *body) {
    auto i = bodies_.at(body);
    scene_->bodies[i].mass = 0;
    bodies_.erase(body);
    free_bodies_.push(i);
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(RigidBody* body) {
    ym_frame3f frame = ysr_get_transform(scene_, bodies_.at(body));

    return std::make_pair(
        Vec3(frame.t[0], frame.t[1], frame.t[2]),
        Quaternion(Mat3(frame.m.data()))
    );
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Controller("rigid-body"),
    simulation_(simulation) {

    object_ = dynamic_cast<MoveableObject*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }

    body_id_ = simulation->acquire_body(this);
}

RigidBody::~RigidBody() {
    simulation_->release_body(this);
}

void RigidBody::add_force(const Vec3 &force) {
    force_ += force;
}

void RigidBody::do_post_update(double dt) {
    auto xform = simulation_->body_transform(this);
    object_->set_absolute_position(xform.first);
    object_->set_absolute_rotation(xform.second);
}

}
}
