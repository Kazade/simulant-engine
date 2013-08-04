#include "../../generic/managed.h"

#include "ode_engine.h"
#include "ode_body.h"
#include "ode_collidable.h"

namespace kglt {
namespace physics {

std::shared_ptr<ResponsiveBody> ODEEngine::new_responsive_body(Object* owner) {
    auto result = std::make_shared<ODEBody>(owner);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}

std::shared_ptr<Collidable> ODEEngine::new_collidable(Object* owner) {
    auto result = std::make_shared<ODECollidable>(owner);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}


}
}
