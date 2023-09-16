#pragma once

#include "scene.h"
#include "../application.h"
#include "../behaviours/physics/simulation.h"

namespace smlt {

class PhysicsScene: public Scene {
protected:
    PhysicsScene(Window* window):
        Scene(window) {}

    virtual void on_fixed_update(float step) {
        if(physics_) {
            physics_->fixed_update(step);
        }
    }

private:
    void pre_load() override {
        physics_ = smlt::behaviours::RigidBodySimulation::create(get_app()->time_keeper);
    }

    void post_unload() override {
        physics_.reset();
    }

    std::shared_ptr<smlt::behaviours::RigidBodySimulation> physics_;

protected:
    S_DEFINE_PROPERTY(physics, &PhysicsScene::physics_);
};

}
