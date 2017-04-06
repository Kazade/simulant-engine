#pragma once

#include "body.h"

namespace smlt {
namespace controllers {

/*
 * Almost the same as a rigid body, but has no mass, and doesn't take part in the simulation
 * aside from acting as a collider */
class StaticBody:
    public impl::Body,
    public Managed<StaticBody> {

public:
    StaticBody(Controllable* object, RigidBodySimulation *simulation, ColliderType collider=COLLIDER_TYPE_BOX);
    ~StaticBody();

    using impl::Body::init;
    using impl::Body::cleanup;

    const std::string name() const { return "Static Body"; }
private:
    bool is_dynamic() const override { return false; }
};

}
}
