#pragma once

#include "scene.h"
#include "../application.h"
#include "../behaviours/physics/simulation.h"

namespace smlt {

template<typename T>
class PhysicsScene:
    public Scene<T> {

protected:
    PhysicsScene(Window* window):
        Scene<T>(window) {}

    virtual void _fixed_update_thunk(float step) {
        if(physics_) {
            physics_->fixed_update(step);
        }

        Scene<T>::_fixed_update_thunk(step);
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
