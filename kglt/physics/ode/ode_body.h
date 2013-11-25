#ifndef ODE_BODY_H
#define ODE_BODY_H

#include <ode/ode.h>
#include <unordered_map>

#include "../../kazbase/exceptions.h"
#include "../responsive_body.h"

namespace kglt {
namespace physics {

class ODECollidable;

class ODEBody : public ResponsiveBody {
public:
    ODEBody(Object* owner):
        ResponsiveBody(owner) {}

    friend class ODECollidable;

private:
    std::unordered_map<ConstraintID, dJointID> constraints_;

    dBodyID body_;
    dMass mass_;

    bool do_init();
    void do_cleanup();

    virtual void do_set_position(const kglt::Vec3& position);
    virtual kglt::Vec3 do_position() const;

    virtual void do_set_rotation(const kglt::Quaternion& rotation);
    virtual kglt::Quaternion do_rotation() const;

    virtual void do_apply_linear_force_global(const kglt::Vec3& force);
    virtual void do_apply_linear_force_local(const kglt::Vec3& force);

    virtual void do_apply_angular_force_global(const kglt::Vec3& force);
    virtual void do_apply_angular_force_local(const kglt::Vec3& force);

    virtual void do_set_angular_damping(const float amount);
    virtual void do_set_linear_damping(const float amount);

    virtual void do_set_angular_velocity(const kglt::Vec3& velocity);
    virtual kglt::Vec3 do_angular_velocity() const;

    virtual void do_set_linear_velocity(const kglt::Vec3& vel);
    virtual kglt::Vec3 do_linear_velocity() const;

    void do_set_mass_sphere(float total_mass, float radius);
    void do_set_mass_box(float total_mass, float width, float height, float depth);

    virtual ConstraintID do_create_fixed_constraint(ResponsiveBody& other);
    virtual void do_destroy_constraint(ConstraintID c);
    virtual void do_enable_constraint(ConstraintID c);
    virtual void do_disable_constraint(ConstraintID c);
};

}
}

#endif // ODE_BODY_H
