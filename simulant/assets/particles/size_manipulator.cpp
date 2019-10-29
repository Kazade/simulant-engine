#include "size_manipulator.h"

#include "../../nodes/particle_system.h"

namespace smlt {

void SizeManipulator::do_manipulate(ParticleSystem* system, Particle* particles, std::size_t particle_count, float dt) {
    /* We always have to set the curve before manipulation to take into
         * account any scaling of the particle system */

    if(is_bell_curve_) {
        /* We have to only respect X scale here, no other option! */
        Manipulator::set_bell_curve(
            peak_ * system->scale().x,
            deviation_
        );
    } else if(is_linear_curve_) {
        Manipulator::set_linear_curve(
            rate_* system->scale().x
        );
    }

    Particle* particle = particles;
    for(auto i = 0u; i < particle_count; ++i) {
        float e = (particle->lifetime - particle->ttl);
        float n =  e / particle->lifetime;
        particle->dimensions.x = curve_(particle->initial_dimensions.x, n, e);
        particle->dimensions.y = curve_(particle->initial_dimensions.y, n, e);

        ++particle;
    }
}

}
