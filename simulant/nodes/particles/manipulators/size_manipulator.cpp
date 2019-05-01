#include "size_manipulator.h"
#include "../../particle_system.h"

namespace smlt {
namespace particles {

void SizeManipulator::do_manipulate(std::vector<Particle> &particles, float dt) {
    /* We always have to set the curve before manipulation to take into
         * account any scaling of the particle system */

    if(is_bell_curve_) {
        /* We have to only respect X scale here, no other option! */
        Manipulator::set_bell_curve(
            peak_ * particle_system_->scale().x,
            deviation_
        );
    } else if(is_linear_curve_) {
        Manipulator::set_linear_curve(
            rate_* particle_system_->scale().x
        );
    }

    for(auto& particle: particles) {
        float e = (particle.lifetime - particle.ttl);
        float n =  e / particle.lifetime;
        particle.dimensions.x = curve_(particle.initial_dimensions.x, n, e);
        particle.dimensions.y = curve_(particle.initial_dimensions.y, n, e);
    }
}

}
}
