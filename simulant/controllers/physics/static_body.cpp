#include "static_body.h"

namespace smlt {
namespace controllers {

StaticBody::StaticBody(Controllable* object, RigidBodySimulation* simulation, ColliderType collider):
    Body(object, simulation, collider) {

}

StaticBody::~StaticBody() {

}


}
}
