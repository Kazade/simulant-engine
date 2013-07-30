#include "../../generic/managed.h"

#include "ode_engine.h"
#include "ode_body.h"

namespace kglt {
namespace physics {

std::shared_ptr<PhysicsBody> ODEEngine::new_body(Actor* owner) {
    auto result = std::make_shared<ODEBody>(owner);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}

}
}
