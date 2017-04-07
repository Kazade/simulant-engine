/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <queue>

#include "./body.h"

#include "../../generic/managed.h"
#include "../../types.h"
#include "../../nodes/stage_node.h"
#include "../../time_keeper.h"


struct b3World;
struct b3Body;
struct b3Hull;

namespace smlt {

namespace controllers {

namespace impl {
    class Body;
}

class RigidBody;

/*
 * A rigid body controller
 */
class RigidBody:
    public impl::Body,
    public Managed<RigidBody> {

public:
    RigidBody(Controllable* object, RigidBodySimulation *simulation, GeneratedColliderType collider=GENERATED_COLLIDER_TYPE_NONE);
    ~RigidBody();

    void add_force(const Vec3& force);
    void add_force_at_position(const Vec3& force, const Vec3& position);
    void add_relative_force(const Vec3& force);

    void add_torque(const Vec3& torque);
    void add_relative_torque(const Vec3& torque);

    void add_impulse(const Vec3& impulse);
    void add_impulse_at_position(const Vec3& impulse, const Vec3& position);

    float mass() const;
    Vec3 linear_velocity() const;
    Vec3 linear_velocity_at(const Vec3& position) const;

    using impl::Body::init;
    using impl::Body::cleanup;

    Vec3 position() const;
    Quaternion rotation() const;

    Vec3 forward() {
        Quaternion rot = rotation();
        return Vec3::NEGATIVE_Z * rot;
    }

    const std::string name() const { return "Rigid Body"; }

private:
    friend class RigidBodySimulation;

    // Cleared each step
    Vec3 force_;
    Vec3 torque_;
};

}
}
