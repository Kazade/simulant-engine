#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

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
};

}

#endif // PHYSICS_ENGINE_H
