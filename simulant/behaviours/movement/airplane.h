#pragma once

#include "../behaviour.h"
#include "../../event_listener.h"

namespace smlt {
namespace behaviours {

class Airplane:
    public BehaviourWithInput,
    public RefCounted<Airplane> {

public:
    Airplane(Core* core);

    void set_turn_speed(float x) { turn_speed_ = x; }

    void fixed_update(float step);

    const std::string name() const { return "Airplane"; }

private:
    void on_behaviour_added(Organism *controllable);

    Core* core_;

    float turn_speed_ = 10.0f;
    float throttle_speed_ = 200.0f;
    float throttle_ = 0.0f;
};


}
}
