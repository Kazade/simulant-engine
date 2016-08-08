#include "rigid_body.h"


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

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Controller("rigid-body"),
    simulation_(simulation) {

    body_id_ = simulation->acquire_body(this);
}

RigidBody::~RigidBody() {
    simulation_->release_body(this);
}

void RigidBody::add_force(const Vec3 &force) {
    force_ += force;
}

}
}
