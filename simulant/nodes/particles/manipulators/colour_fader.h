#pragma once

#include "../manipulator.h"
#include "curves.h"

namespace smlt {
namespace particles {

class ColourFader:
    public Manipulator {
public:
    ColourFader(const std::vector<Colour>& colours):
        Manipulator("colour_fader") {}

private:
    void do_manipulate(std::vector<Particle>& particles, float dt) {
        auto size = colours_.size();
        for(auto& particle: particles) {
            float e = (particle.lifetime - particle.ttl);
            float n = e / particle.lifetime;
            int colour = (size * n);
            particle.colour = colours_[colour];
        }
    }

    std::vector<Colour> colours_;
};

}
}
