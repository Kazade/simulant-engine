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

#ifndef BASE_H
#define BASE_H

#include "stage.h"
#include "window_base.h"
#include "types.h"

namespace smlt {

class ActorHolder {
public:
    ActorHolder(WindowBase& window):
        window_(window) {}

    virtual StageID stage_id() const = 0;
    virtual ActorID actor_id() const = 0;

    StagePtr stage() { return window_.stage(stage_id()); }
    ActorPtr actor() { return stage()->actor(actor_id()); }

    const StagePtr stage() const { return window_.stage(stage_id()); }
    const ActorPtr actor() const { return stage()->actor(actor_id()); }

private:
    WindowBase& window_;
};

class MoveableActorHolder:
    public ActorHolder {

public:
    MoveableActorHolder(WindowBase &window);

    void set_position(const smlt::Vec3& position);
    smlt::Vec3 position() const;

    void set_rotation(const Quaternion &quaternion);
    Quaternion rotation() const;

    virtual void set_velocity(const smlt::Vec3& vel);
    virtual smlt::Vec3 velocity() const;

    virtual void set_mass(float mass);
    virtual float mass() const;

    virtual void set_min_speed(float speed);
    virtual float min_speed() const;

    virtual void set_max_speed(float speed);
    virtual float max_speed() const;

    virtual void set_max_force(float force);
    virtual float max_force() const;

    virtual void set_radius(float radius);
    virtual float radius() const;

private:
    smlt::Vec3 velocity_;
    float mass_ = 1.0;
    float max_speed_ = 1.0;
    float min_speed_ = 0.0;
    float max_force_ = 1.0;
    float radius_ = 1.0;
};

}

#endif // BASE_H
