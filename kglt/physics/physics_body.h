#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include "../types.h"

namespace kglt {

class Actor;

class PhysicsBody {
public:
    PhysicsBody(Actor* owner):
        owner_(owner) {}

    virtual ~PhysicsBody() {}

    bool init();
    void cleanup();

    virtual void set_position(const kglt::Vec3& position) = 0;
    virtual kglt::Vec3 position() const = 0;

    virtual void set_rotation(const kglt::Quaternion& rotation) = 0;
    virtual kglt::Quaternion rotation() const = 0;

    //FIXME: mass distribution?
    virtual void set_mass(float mass) = 0;
    virtual float mass() const = 0;

    //FIXME: Don't know if these belong here, depends if physics engines
    // support limiting things like this...
    virtual void set_max_speed(float speed_limit) = 0;
    virtual float max_speed() const = 0;

    virtual void set_min_speed(float speed_limit) = 0;
    virtual float min_speed() const = 0;
    //ENDFIXME

    virtual void apply_linear_force_global(const kglt::Vec3& force) = 0;
    virtual void apply_linear_force_local(const kglt::Vec3& force) = 0;

    virtual void apply_angular_force_global(const kglt::Vec3& force) = 0;
    virtual void apply_angular_force_local(const kglt::Vec3& force) = 0;

    virtual void set_angular_damping(const float amount) = 0;

    virtual void set_angular_velocity(const kglt::Vec3& velocity) = 0;
    virtual kglt::Vec3 angular_velocity() const = 0;

    virtual void set_linear_velocity(const kglt::Vec3& velocity) = 0;
    virtual kglt::Vec3 linear_velocity() const = 0;

    Actor* owner() { return owner_; }
private:
    Actor* owner_;
};

}

#endif // PHYSICS_BODY_H
