#include "static_body.h"

namespace smlt {
namespace controllers {

StaticBody::StaticBody(Controllable* object, RigidBodySimulation* simulation):
    Body(object, simulation) {

}

StaticBody::~StaticBody() {

}


}
}
