#ifndef RESPONSIVE_BODY_H
#define RESPONSIVE_BODY_H

#include "../generic/unique_id.h"
#include "../types.h"

namespace kglt {

class Object;
class PhysicsEngine;

class ResponsiveBody {
public:
    ResponsiveBody(Object* owner);
    virtual ~ResponsiveBody() { }

    bool init();
    void cleanup();

    void set_position(const kglt::Vec3& position);
    kglt::Vec3 position() const;

    void set_rotation(const kglt::Quaternion& rotation);
    kglt::Quaternion rotation() const;

    void set_mass_sphere(float total_mass, float radius);
    void set_mass_box(float total_mass, float width, float height, float depth);

    void apply_linear_impulse_global(const kglt::Vec3& impulse);
    void apply_angular_impulse_global(const kglt::Vec3& impulse);
    void apply_linear_impulse_local(const kglt::Vec3& impulse);
    void apply_angular_impulse_local(const kglt::Vec3& impulse);

    void apply_linear_force_global(const kglt::Vec3& force);
    void apply_linear_force_local(const kglt::Vec3& force);

    void apply_angular_force_global(const kglt::Vec3& force);
    void apply_angular_force_local(const kglt::Vec3& force);

    void set_angular_damping(const float amount);
    void set_linear_damping(const float amount);

    void set_angular_velocity(const kglt::Vec3& velocity);
    kglt::Vec3 angular_velocity() const;

    void set_linear_velocity(const kglt::Vec3& velocity);
    kglt::Vec3 linear_velocity() const;

    Object* owner() { return owner_; }
    PhysicsEnginePtr engine() { return engine_; }

    ConstraintID create_fixed_constraint(ResponsiveBody& other);
    ConstraintID create_pivot_constraint(ResponsiveBody& other, const kglt::Vec3& pivot);
    void destroy_constraint(ConstraintID c);
    void enable_constraint(ConstraintID c);
    void disable_constraint(ConstraintID c);

private:    
    Object* owner_;
    PhysicsEnginePtr engine_;

    virtual bool do_init() = 0;
    virtual void do_cleanup() = 0;

    virtual void do_set_position(const kglt::Vec3& position) = 0;
    virtual kglt::Vec3 do_position() const = 0;

    virtual void do_set_rotation(const kglt::Quaternion& rotation) = 0;
    virtual kglt::Quaternion do_rotation() const = 0;

    virtual void do_set_mass_sphere(float total_mass, float radius) = 0;
    virtual void do_set_mass_box(float total_mass, float width, float height, float depth) = 0;

    virtual void do_apply_linear_force_global(const kglt::Vec3& force) = 0;
    virtual void do_apply_linear_impulse_global(const kglt::Vec3& impulse) = 0;
    virtual void do_apply_linear_force_local(const kglt::Vec3& force) = 0;
    virtual void do_apply_linear_impulse_local(const kglt::Vec3& impulse) = 0;

    virtual void do_apply_angular_force_global(const kglt::Vec3& force) = 0;
    virtual void do_apply_angular_force_local(const kglt::Vec3& force) = 0;
    virtual void do_apply_angular_impulse_global(const kglt::Vec3& impulse) = 0;
    virtual void do_apply_angular_impulse_local(const kglt::Vec3& impulse) = 0;

    virtual void do_set_angular_damping(const float amount) = 0;
    virtual void do_set_linear_damping(const float amount) = 0;

    virtual void do_set_angular_velocity(const kglt::Vec3& velocity) = 0;
    virtual kglt::Vec3 do_angular_velocity() const = 0;

    virtual void do_set_linear_velocity(const kglt::Vec3& velocity) = 0;
    virtual kglt::Vec3 do_linear_velocity() const = 0;

    virtual ConstraintID do_create_fixed_constraint(ResponsiveBody& other) = 0;
    virtual ConstraintID do_create_pivot_constraint(ResponsiveBody& other, const kglt::Vec3& pivot) {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    virtual void do_destroy_constraint(ConstraintID c) = 0;
    virtual void do_enable_constraint(ConstraintID c) = 0;
    virtual void do_disable_constraint(ConstraintID c) = 0;

};

}

#endif // PHYSICS_BODY_H
