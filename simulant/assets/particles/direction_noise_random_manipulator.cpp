#include "direction_noise_random_manipulator.h"

#include "../../nodes/particle_system.h"
#include "../../macros.h"

namespace smlt {

void DirectionNoiseRandomManipulator::do_manipulate(ParticleSystem *system, Particle *particles, std::size_t particle_count, float dt) const {
    _S_UNUSED(system);

    Particle* it = particles;
    while(particle_count--) {
        smlt::Vec3 noise = smlt::Vec3(
            (rand() % 100 - 50) * noise_amount_.x,
            (rand() % 100 - 50) * noise_amount_.y,
            (rand() % 100 - 50) * noise_amount_.z
        );

        auto final_dir = dir_ + noise;
        it->position += final_dir * dt;

        auto rot = Quaternion::look_rotation(final_dir);
        it->rotation = rot * dt;

        ++it;
    }
}


}