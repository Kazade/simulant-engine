#ifndef ODE_ENGINE_H
#define ODE_ENGINE_H

#include <memory>
#include <unordered_map>

#include <ode/ode.h>

#include "../types.h"
#include "../../generic/managed.h"
#include "../physics_engine.h"

namespace kglt {
namespace physics {

class ODEBody;
class ODECollidable;

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
    std::shared_ptr<ResponsiveBody> new_responsive_body(kglt::Object *owner);
    std::shared_ptr<Collidable> new_collidable(kglt::Object *owner);

    ShapeID create_plane(float a, float b, float c, float d);
    void set_gravity(const Vec3& gravity);

    friend class ODEBody;
    friend class ODECollidable;
private:
    //Used by ODEBody
    dWorldID world() const { return world_; }

    dWorldID world_;
    dSpaceID space_;

    std::unordered_map<ShapeID, dGeomID> shapes_;
};

}
}

#endif // ODE_ENGINE_H
