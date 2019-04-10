#pragma once

#include "../manipulator.h"
#include "curves.h"

namespace smlt {
namespace particles {

class SizeManipulator : public Manipulator {
public:
    SizeManipulator():
        Manipulator("scalar") {}


private:
    void do_manipulate(std::vector<Particle>& particles, float dt) {
        for(auto& particle: particles) {
            float e = (particle.lifetime - particle.ttl);
            float n =  e / particle.lifetime;
            particle.dimensions.x = curve_(particle.initial_dimensions.x, n, e);
            particle.dimensions.y = curve_(particle.initial_dimensions.y, n, e);
        }
    }
};


}
}
