#ifndef RESPONSIVE_BODY_H
#define RESPONSIVE_BODY_H

#include "../generic/unique_id.h"
#include "../types.h"

namespace kglt {

class Object;

class ResponsiveBody {
public:
    ResponsiveBody(Object* owner):
        owner_(owner) {}

    virtual ~ResponsiveBody() {}

    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual void set_position(const kglt::Vec3& position) = 0;
    virtual kglt::Vec3 position() const = 0;

    virtual void set_rotation(const kglt::Quaternion& rotation) = 0;
    virtual kglt::Quaternion rotation() const = 0;

    void set_mass_sphere(float total_mass, float radius) {
        //FIXME: Lock
        do_set_mass_sphere(total_mass, radius);
    }

    void set_mass_box(float total_mass, float width, float height, float depth) {
        //FIXME: Lock
        do_set_mass_box(total_mass, width, height, depth);
    }

    virtual void apply_linear_force_global(const kglt::Vec3& force) = 0;
    virtual void apply_linear_force_local(const kglt::Vec3& force) = 0;

    virtual void apply_angular_force_global(const kglt::Vec3& force) = 0;
    virtual void apply_angular_force_local(const kglt::Vec3& force) = 0;

    virtual void set_angular_damping(const float amount) = 0;

    virtual void set_angular_velocity(const kglt::Vec3& velocity) = 0;
    virtual kglt::Vec3 angular_velocity() const = 0;

    virtual void set_linear_velocity(const kglt::Vec3& velocity) = 0;
    virtual kglt::Vec3 linear_velocity() const = 0;

    Object* owner() { return owner_; }

    virtual ConstraintID create_fixed_constaint(ResponsiveBody& other) = 0;
    virtual ConstraintID create_pivot_constraint(ResponsiveBody& other, const kglt::Vec3& pivot) {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    virtual void destroy_constraint(ConstraintID c) = 0;
    virtual void enable_constraint(ConstraintID c) = 0;
    virtual void disable_constraint(ConstraintID c) = 0;

private:
    Object* owner_;

    virtual void do_set_mass_sphere(float total_mass, float radius) = 0;
    virtual void do_set_mass_box(float total_mass, float width, float height, float depth) = 0;
};

}

#endif // PHYSICS_BODY_H
