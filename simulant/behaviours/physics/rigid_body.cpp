//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "bounce/bounce.h"

#include "rigid_body.h"
#include "simulation.h"
#include "../../nodes/actor.h"
#include "../../stage.h"


// These are just to keep Bounce happy
bool b3PushProfileScope(char const*) { return false; }
void b3PopProfileScope() {}


namespace smlt {
namespace behaviours {


RigidBody::RigidBody(RigidBodySimulation* simulation):
    DynamicBody(simulation) {

}

RigidBody::~RigidBody() {

}

void RigidBody::set_center_of_mass(const smlt::Vec3& com) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3MassData data;
    b->GetMassData(&data);
    to_b3vec3(com, data.center);
    b->SetMassData(&data);
}

float RigidBody::mass() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return 0;
    }

    const b3Body* b = sim->bodies_.at(this);
    return b->GetMass();
}



}
}
