#include "size_manipulator.h"

#include "curves.h"
#include "../../nodes/particle_system.h"

namespace smlt {

void SizeManipulator::do_manipulate(ParticleSystem* system, Particle* particles, std::size_t particle_count, float dt) const {
    /* We always have to set the curve before manipulation to take into
         * account any scaling of the particle system */

    auto create_scaled_curve = [this](float size) -> CurveFunc {
        using namespace std::placeholders;

        if(is_bell_curve_) {
            return std::bind(&bell_curve, _1, _2, _3, peak_ * size, deviation_);
        } else {
            assert(is_linear_curve_);
            return std::bind(&linear_curve, _1, _2, _3, rate_ * size);
        }
    };


    /* We have to only respect X scale here, no other option! */
    auto curve = create_scaled_curve(system->scale().x);

    Particle* particle = particles;
    for(auto i = 0u; i < particle_count; ++i) {
        float e = (particle->lifetime - particle->ttl);
        float n =  e / particle->lifetime;
        particle->dimensions.x = curve(particle->initial_dimensions.x, n, e);
        particle->dimensions.y = curve(particle->initial_dimensions.y, n, e);

        ++particle;
    }
}

}
