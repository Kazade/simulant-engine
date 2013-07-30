#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

namespace kglt {

class PhysicsBody;
class Actor;

class PhysicsEngine {
public:
    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual void step(double dt) = 0;

    //Factory function
    virtual std::shared_ptr<PhysicsBody> new_body(Actor* owner) = 0;
};

}

#endif // PHYSICS_ENGINE_H
