#ifndef ODE_BODY_H
#define ODE_BODY_H

#include <ode/ode.h>

#include "../../kazbase/exceptions.h"
#include "../physics_body.h"

namespace kglt {
namespace physics {

class ODEBody : public PhysicsBody {
public:
    ODEBody(Object* owner):
        PhysicsBody(owner) {}

    bool init();
    void cleanup();

    virtual void set_position(const kglt::Vec3& position);
    virtual kglt::Vec3 position() const;

    virtual void set_rotation(const kglt::Quaternion& rotation);
    virtual kglt::Quaternion rotation() const;

    //FIXME: mass distribution?
    virtual void set_mass(float mass);
    virtual float mass() const;

    //FIXME: Don't know if these belong here, depends if physics engines
    // support limiting things like this...
    virtual void set_max_speed(float speed_limit);
    virtual float max_speed() const;

    virtual void set_min_speed(float speed_limit);
    virtual float min_speed() const;
    //ENDFIXME

    virtual void apply_linear_force_global(const kglt::Vec3& force);
    virtual void apply_linear_force_local(const kglt::Vec3& force);

    virtual void apply_angular_force_global(const kglt::Vec3& force);
    virtual void apply_angular_force_local(const kglt::Vec3& force);

    virtual void set_angular_damping(const float amount);

    virtual void set_angular_velocity(const kglt::Vec3& velocity);
    virtual kglt::Vec3 angular_velocity() const;

    virtual void set_linear_velocity(const kglt::Vec3& velocity);
    virtual kglt::Vec3 linear_velocity() const;

    virtual void add_cube(float width) { throw NotImplementedError(__FILE__, __LINE__); }
    virtual void add_sphere(float radius) { throw NotImplementedError(__FILE__, __LINE__); }

private:
    dBodyID body_;
    dMass mass_;
};

}
}

#endif // ODE_BODY_H
