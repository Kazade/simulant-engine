#pragma once

#include "rigid_body.h"

namespace kglt {
namespace controllers {

class RaycastVehicle:
    public RigidBody,
    public Managed<RaycastVehicle> {


};


}
}
