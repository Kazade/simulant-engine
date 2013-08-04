#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include "../types.h"

namespace kglt {

class ResponsiveBody;
class Collidable;

class Object;

class PhysicsEngine {
public:
    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual void step(double dt) = 0;

    //Factory function
    virtual std::shared_ptr<ResponsiveBody> new_responsive_body(Object* owner) = 0;
    virtual std::shared_ptr<Collidable> new_collidable(Object* owner) = 0;

    virtual ShapeID create_plane(float a, float b, float c, float d) = 0;
    virtual void set_gravity(const kglt::Vec3& gravity) = 0;
};

ShapeID get_next_shape_id();

}

#endif // PHYSICS_ENGINE_H
