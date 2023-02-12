#pragma once

#include "../particle_script.h"

namespace smlt {

class DirectionNoiseRandomManipulator : public Manipulator {
public:
    DirectionNoiseRandomManipulator(ParticleScript* script, const Vec3& dir, const Vec3& noise_amount):
        Manipulator(script, "direction_noise_random"),
        dir_(dir),
        noise_amount_(noise_amount) {}

private:
    void do_manipulate(
        ParticleSystem* system,
        Particle* particles,
        std::size_t particle_count, float dt) const override;

    Vec3 dir_;
    Vec3 noise_amount_;
};


}