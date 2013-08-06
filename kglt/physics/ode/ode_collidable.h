#ifndef ODE_COLLIDABLE_H
#define ODE_COLLIDABLE_H

#include <ode/ode.h>

#include <unordered_map>

#include "../collidable.h"

namespace kglt {
namespace physics {

class ODECollidable : public Collidable {
public:
    ODECollidable(Object* owner, PhysicsEngine* engine):
        Collidable(owner, engine) {}

    virtual bool init();
    virtual void cleanup();

    virtual ShapeID add_sphere(float radius);
    virtual ShapeID add_box(float width, float height, float depth);
    virtual ShapeID add_capsule(float radius, float length);
    virtual ShapeID add_plane(float a, float b, float c, float d);

    virtual void set_relative_position(ShapeID shape, const Vec3& pos);
    virtual void set_absolute_position(ShapeID shape, const Vec3& pos);

    virtual void set_relative_rotation(ShapeID shape, const Quaternion& quat);
    virtual void set_absolute_rotation(ShapeID shape, const Quaternion& quat);

    virtual void attach_to_responsive_body(ResponsiveBody& body);

    virtual void set_bounciness(float value);
    virtual float bounciness() const;

    virtual void set_friction(int32_t friction);
    virtual int32_t friction() const;

private:
    friend class ODEEngine;

    std::unordered_map<ShapeID, dGeomID> shapes_;

    dSpaceID get_space();

    float bounciness_ = 0.5;
    int32_t friction_ = 100000;
};

}
}

#endif // ODE_COLLIDABLE_H
