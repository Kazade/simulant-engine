#include "static_body.h"

namespace smlt {
namespace controllers {

StaticBody::StaticBody(Controllable* object, RigidBodySimulation* simulation, GeneratedColliderType collider):
    Body(object, simulation, collider) {

}

StaticBody::~StaticBody() {

}


}
}
