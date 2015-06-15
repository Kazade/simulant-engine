#include "actor.h"
#include "base.h"

namespace kglt {

MoveableActorHolder::MoveableActorHolder(WindowBase& window):
    ActorHolder(window) {

}

void MoveableActorHolder::set_position(const Vec3& position) {
    actor()->set_absolute_position(position);
}

Vec3 MoveableActorHolder::position() const {
    return actor()->absolute_position();
}

void MoveableActorHolder::set_rotation(const Quaternion &quaternion) {
    actor()->set_absolute_rotation(quaternion);
}

Quaternion MoveableActorHolder::rotation() const {
    return actor()->absolute_rotation();
}

void MoveableActorHolder::set_velocity(const kglt::Vec3& vel) {
    velocity_ = vel;
}

kglt::Vec3 MoveableActorHolder::velocity() const {
    return velocity_;
}

void MoveableActorHolder::set_mass(float mass) {
    mass_ = mass;
}

float MoveableActorHolder::mass() const {
    return mass_;
}

void MoveableActorHolder::set_max_speed(float speed) {
    max_speed_ = speed;
}

float MoveableActorHolder::max_speed() const {
    return max_speed_;
}

void MoveableActorHolder::set_min_speed(float speed) {
    min_speed_ = speed;
}

float MoveableActorHolder::min_speed() const {
    return min_speed_;
}

void MoveableActorHolder::set_max_force(float force) {
    max_force_ = force;
}

float MoveableActorHolder::max_force() const {
    return max_force_;
}

void MoveableActorHolder::set_radius(float radius) {
    radius_ = radius;
}

float MoveableActorHolder::radius() const {
    return radius_;
}

}
