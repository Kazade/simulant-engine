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
    ysr_advance(scene_, dt);
}

uint32_t RigidBodySimulation::acquire_body() {
    uint32_t i = free_bodies_.front();
    free_bodies_.pop();

    scene_->bodies[i].mass = 1.0; // Enable the body in the simulation
    return i;
}

void RigidBodySimulation::release_body(uint32_t i) {
    scene_->bodies[i].mass = 0;
    free_bodies_.push(i);
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Controller("rigid-body"),
    simulation_(simulation) {

    body_id_ = simulation->acquire_body();
}

RigidBody::~RigidBody() {
    simulation_->release_body(body_id_);
}

}
}
