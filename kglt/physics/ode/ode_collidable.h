#ifndef ODE_COLLIDABLE_H
#define ODE_COLLIDABLE_H

#include <ode/ode.h>

#include <unordered_map>

#include "../collidable.h"

namespace kglt {
namespace physics {

class ODECollidable : public Collidable {
public:
    ODECollidable(Object* owner):
        Collidable(owner) {}

    virtual bool init();
    virtual void cleanup();

    virtual ShapeID add_sphere(float radius);
    virtual ShapeID add_box(float width, float height, float depth);
    virtual ShapeID add_capsule(float radius, float length);

    virtual void set_relative_position(ShapeID shape, const Vec3& pos);
    virtual void set_absolute_position(ShapeID shape, const Vec3& pos);

    virtual void set_relative_rotation(ShapeID shape, const Quaternion& quat);
    virtual void set_absolute_rotation(ShapeID shape, const Quaternion& quat);

    virtual void attach_to_responsive_body(ResponsiveBody& body);

private:
    std::unordered_map<ShapeID, dGeomID> shapes_;

    dSpaceID get_space();
};

}
}

#endif // ODE_COLLIDABLE_H
