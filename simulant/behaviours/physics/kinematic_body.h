
#pragma once

#include "body.h"
#include "simulation.h"

namespace smlt {
namespace behaviours {

class KinematicBody:
    public impl::Body,
    public RefCounted<KinematicBody> {

public:
    KinematicBody(RigidBodySimulation* simulation);
    virtual ~KinematicBody();

    using impl::Body::init;
    using impl::Body::clean_up;

    const char* name() const { return "Kinematic Body"; }
private:
    virtual bool is_kinematic() const { return true; }

    friend class RigidBodySimulation;
};

}
}
