#pragma once

#include "../particle_script.h"
#include "../../nodes/particles/particle.h"

#include "curves.h"

namespace smlt {

class AlphaFader:
    public Manipulator {
public:
    AlphaFader(ParticleScript* script, const std::vector<float>& alphas, bool interpolate):
        Manipulator(script, "alpha_fader"),
        alphas_(alphas),
        interpolate_(interpolate) {}

private:
    void do_manipulate(ParticleSystem*, Particle* particles, std::size_t particle_count, float) const {
        auto size = (float) alphas_.size();
        float fsize = float(size);
        Particle* particle = particles;
        for(auto i = 0u; i < particle_count; ++i, ++particle) {
            const float e = (particle->lifetime - particle->ttl);
            const float n = smlt::fast_divide(e, particle->lifetime);
            const float fsizen = fsize * n;

            uint8_t colour = smlt::clamp(fsizen, 0.0f, fsize);

            particle->colour.a = alphas_[colour];

            if(interpolate_) {
                const float f = fsizen - std::floor(fsizen);
                auto next_alpha = alphas_[std::min((uint32_t) colour + 1, (uint32_t) size - 1)];
                particle->colour.a = (particle->colour.a * (1.0f - f)) + (next_alpha * f);
            }
        }
    }

    std::vector<float> alphas_;
    bool interpolate_ = true;
};

}
