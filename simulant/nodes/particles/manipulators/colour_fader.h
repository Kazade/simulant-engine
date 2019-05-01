#pragma once

#include "../manipulator.h"
#include "curves.h"

namespace smlt {
namespace particles {

class ColourFader:
    public Manipulator {
public:
    ColourFader(ParticleSystem* system, const std::vector<Colour>& colours, bool interpolate):
        Manipulator(system, "colour_fader"),
        colours_(colours),
        interpolate_(interpolate) {}

private:
    void do_manipulate(std::vector<Particle>& particles, float dt) {
        auto size = colours_.size();
        for(auto& particle: particles) {
            float e = (particle.lifetime - particle.ttl);
            float n = e / particle.lifetime;
            long unsigned int colour = (size * n);

            particle.colour = colours_[colour];

            if(interpolate_) {
                float f = (size * n) - int(size * n);
                auto next_colour = colours_[std::min((uint32_t) colour + 1, (uint32_t) size - 1)];
                particle.colour = (particle.colour * (1.0f - f)) + (next_colour * f);
            }
        }
    }

    std::vector<Colour> colours_;
    bool interpolate_ = true;
};

}
}
