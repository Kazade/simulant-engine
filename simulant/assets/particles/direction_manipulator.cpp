#include "direction_manipulator.h"
#include "../../nodes/particle_system.h"

namespace smlt {

void DirectionManipulator::do_manipulate(ParticleSystem *system, Particle *particles, std::size_t particle_count, float dt) const {
    _S_UNUSED(system);

    Particle* it = particles;
    while(particle_count--) {
        it->position += dir_ * dt;
        ++it;
    }
}

}
