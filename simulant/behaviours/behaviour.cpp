#include "behaviour.h"

namespace smlt {

void Behaviour::enable() {
    is_enabled_ = true;
}

void Behaviour::disable() {
    is_enabled_ = false;
    first_update_done_ = false;
}

void Behaviour::_update_thunk(float dt) {
    if(!is_enabled_) {
        return;
    }

    Updateable::_update_thunk(dt);
}

void Behaviour::_late_update_thunk(float dt) {
    if(!is_enabled_) {
        return;
    }

    Updateable::_late_update_thunk(dt);
}

void Behaviour::_fixed_update_thunk(float step) {
    if(!is_enabled_) {
        return;
    }

    Updateable::_fixed_update_thunk(step);
}

Organism::~Organism() {
    for(auto& b: behaviours_) {
        b->set_organism(nullptr);
    }

    behaviours_.clear();
    behaviour_names_.clear();
    behaviour_types_.clear();
}

}
