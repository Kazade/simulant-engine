/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <queue>

#include "./dynamic_body.h"

#include "../../generic/managed.h"
#include "../../types.h"
#include "../../nodes/stage_node.h"
#include "../../time_keeper.h"

namespace smlt {
namespace behaviours {

/*
 * A rigid body controller
 */
class RigidBody:
    public impl::DynamicBody,
    public RefCounted<RigidBody> {

public:
    RigidBody(RigidBodySimulation *simulation);
    virtual ~RigidBody();

    using impl::Body::init;
    using impl::Body::clean_up;

    const char* name() const { return "Rigid Body"; }
private:
    friend class RigidBodySimulation;
};

}
}
