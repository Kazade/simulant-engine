#include "stage.h"
#include "particles.h"

namespace kglt {

ParticleSystem::ParticleSystem(Stage* stage, ParticleSystemID id):
    generic::Identifiable<ParticleSystemID>(id),
    ParentSetterMixin<Object>(stage),
    Source(stage) {

}

EmitterPtr ParticleSystem::push_emitter() {
    auto new_emitter = std::make_shared<ParticleEmitter>();
    emitters_.push_back(new_emitter);
    return new_emitter;
}

void ParticleSystem::pop_emitter() {
    emitters_.pop_back();
}

//Boundable entity things
const AABB ParticleSystem::aabb() const {
    if(emitters_.empty()) {
        return AABB();
    }

    AABB result;
    bool first = true;
    for(auto emitter: emitters_) {
        if(emitter->type() == PARTICLE_EMITTER_POINT) {
            auto pos = emitter->relative_position();

            if(pos.x > result.max.x || first) result.max.x = pos.x;
            if(pos.y > result.max.y || first) result.max.y = pos.y;
            if(pos.z > result.max.z || first) result.max.z = pos.z;

            if(pos.x < result.min.x || first) result.min.x = pos.x;
            if(pos.y < result.min.x || first) result.min.y = pos.y;
            if(pos.z < result.min.x || first) result.min.z = pos.z;
        } else {
           throw NotImplementedError(__FILE__, __LINE__);
        }

        first = false;
    }

    return result;
}

const AABB ParticleSystem::transformed_aabb() const {
    AABB box = aabb(); //Get the untransformed one
    auto pos = absolute_position();
    kmVec3Add(&box.min, &box.min, &pos);
    kmVec3Add(&box.max, &box.max, &pos);
    return box;
}

void ParticleSystem::ask_owner_for_destruction() {
    stage()->delete_particle_system(id());
}

}
