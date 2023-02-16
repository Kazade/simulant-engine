#pragma once

#include "../particle_script.h"
#include "curves.h"

namespace smlt {

class DirectionManipulator:
    public Manipulator {

public:
    DirectionManipulator(ParticleScript* script, const smlt::Vec3& dir):
        Manipulator(script, "direction"),
        dir_(dir) {}


private:
    void do_manipulate(
        ParticleSystem* system,
        Particle* particles,
        std::size_t particle_count, float dt) const override;

    smlt::Vec3 dir_;
};


}
