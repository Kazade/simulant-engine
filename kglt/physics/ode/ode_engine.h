#ifndef ODE_ENGINE_H
#define ODE_ENGINE_H

#include <memory>
#include <ode/ode.h>

#include "../../generic/managed.h"
#include "../physics_engine.h"

namespace kglt {
namespace physics {

class ODEBody;

class ODEEngine :
    public PhysicsEngine,
    public Managed<ODEEngine> {

public:
    bool init() {
        dInitODE2(0);
        world_ = dWorldCreate();
        space_ = dHashSpaceCreate(0);
        return true;
    }

    void cleanup() {
        dCloseODE();
    }

    void step(double dt) {
        dWorldStep(world_, dt);
    }

    //Factory function
    std::shared_ptr<PhysicsBody> new_body(kglt::Object *owner);

    friend class ODEBody;
private:
    //Used by ODEBody
    dWorldID world() const { return world_; }

    dWorldID world_;
    dSpaceID space_;
};

}
}

#endif // ODE_ENGINE_H
