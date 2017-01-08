//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "actor.h"
#include "base.h"

namespace smlt {

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

void MoveableActorHolder::set_velocity(const smlt::Vec3& vel) {
    velocity_ = vel;
}

smlt::Vec3 MoveableActorHolder::velocity() const {
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
