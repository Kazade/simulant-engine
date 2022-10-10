#pragma once

#include "../particle_script.h"
#include "curves.h"

namespace smlt {

class ColourFader:
    public Manipulator {
public:
    ColourFader(ParticleScript* script, const std::vector<Colour>& colours, bool interpolate):
        Manipulator(script, "colour_fader"),
        colours_(colours),
        interpolate_(interpolate) {}

private:
    void do_manipulate(ParticleSystem*, Particle* particles, std::size_t particle_count, float) const {
        auto size = (float) colours_.size();
        float fsize = float(size);
        auto particle = particles;
        for(auto i = 0u; i < particle_count; ++i, ++particle) {
            const float e = (particle->lifetime - particle->ttl);
            const float n = smlt::fast_divide(e, particle->lifetime);
            const float fsizen = fsize * n;

            uint8_t colour = smlt::clamp(fsizen, 0.0f, fsize);

            particle->colour = colours_[colour];

            if(interpolate_) {
                const float f = fsizen - std::floor(fsizen);
                auto next_colour = colours_[std::min((uint32_t) colour + 1, (uint32_t) size - 1)];
                particle->colour = (particle->colour * (1.0f - f)) + (next_colour * f);
            }
        }
    }

    std::vector<Colour> colours_;
    bool interpolate_ = true;
};

}
