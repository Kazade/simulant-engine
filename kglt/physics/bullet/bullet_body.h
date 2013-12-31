#ifndef BULLET_BODY_H
#define BULLET_BODY_H

#include <btBulletDynamicsCommon.h>

#include "../../kazbase/exceptions.h"
#include "../responsive_body.h"

namespace kglt {
namespace physics {

class BulletCollidable;

class BulletBody : public ResponsiveBody {
public:
    BulletBody(Object* owner):
        ResponsiveBody(owner) {}

private:
    bool do_init();
    void do_cleanup();

    virtual void do_set_position(const kglt::Vec3& position);
    virtual kglt::Vec3 do_position() const;

    virtual void do_set_rotation(const kglt::Quaternion& rotation);
    virtual kglt::Quaternion do_rotation() const;

    virtual void do_apply_linear_force_global(const kglt::Vec3& force);
    virtual void do_apply_linear_force_local(const kglt::Vec3& force);
    virtual void do_apply_linear_impulse_global(const kglt::Vec3& impulse) override;
    virtual void do_apply_linear_impulse_local(const kglt::Vec3& impulse) override;

    virtual void do_apply_angular_force_global(const kglt::Vec3& force);
    virtual void do_apply_angular_force_local(const kglt::Vec3& force);
    virtual void do_apply_angular_impulse_global(const kglt::Vec3& impulse) override;
    virtual void do_apply_angular_impulse_local(const kglt::Vec3& impulse) override;

    virtual void do_set_angular_damping(const float amount);
    virtual void do_set_linear_damping(const float amount);

    virtual void do_set_angular_velocity(const kglt::Vec3& velocity);
    virtual kglt::Vec3 do_angular_velocity() const;

    virtual void do_set_linear_velocity(const kglt::Vec3& vel);
    virtual kglt::Vec3 do_linear_velocity() const;

    void do_set_mass(float mass, kglt::Vec3 inertia=kglt::Vec3());
    void do_set_mass_sphere(float total_mass, float radius);
    void do_set_mass_box(float total_mass, float width, float height, float depth);

    virtual ConstraintID do_create_fixed_constraint(ResponsiveBody& other);
    virtual void do_destroy_constraint(ConstraintID c);
    virtual void do_enable_constraint(ConstraintID c);
    virtual void do_disable_constraint(ConstraintID c);

    std::unique_ptr<btDefaultMotionState> motion_state_;
    std::unique_ptr<btCompoundShape> compound_shape_;
    std::unique_ptr<btRigidBody> body_;

    friend class BulletCollidable;
};

}
}

#endif // BULLET_BODY_H
