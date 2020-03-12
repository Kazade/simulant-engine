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
        auto size = colours_.size();
        auto particle = particles;
        for(auto i = 0u; i < particle_count; ++i) {
            float e = (particle->lifetime - particle->ttl);
            float n = e / particle->lifetime;
            long unsigned int colour = (size * n);

            particle->colour = colours_[colour];

            if(interpolate_) {
                float f = (size * n) - int(size * n);
                auto next_colour = colours_[std::min((uint32_t) colour + 1, (uint32_t) size - 1)];
                particle->colour = (particle->colour * (1.0f - f)) + (next_colour * f);
            }

            ++particle;
        }
    }

    std::vector<Colour> colours_;
    bool interpolate_ = true;
};

}
