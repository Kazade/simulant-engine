#pragma once

#include "../behaviour.h"

namespace smlt {

class Window;

namespace behaviours {

class RigidBody;
class RigidBodySimulation;

class HoverShip:
    public BehaviourWithInput,
    public RefCounted<HoverShip> {

public:
    HoverShip(Window* window);

    const std::string name() const {
        return "Hover Ship";
    }

    void set_hover_height(float h);
    void set_hover_force(float f);

    void set_speed(float s);
    float speed() const { return speed_; }

    void set_turn_speed(float s);
    float turn_speed() const { return turn_speed_; }

    void update(float dt);
    void fixed_update(float step);

private:
    void on_behaviour_first_update(Organism* owner);

    RigidBody* body_ = nullptr;
    RigidBodySimulation* simulation_ = nullptr;

    float hover_height_ = 3.5f;
    float hover_force_ = 64.0f;
    float turn_speed_ = 5.0f;
    float speed_ = 90.0f;

    float power_input_ = 0.0f;
    float turn_input_ = 0.0f;

protected:
    float power_input() const { return power_input_; }
    void set_power_input(float pi) { power_input_ = pi; }

    float turn_input() const { return turn_input_; }
    void set_turn_input(float ti) { turn_input_ = ti; }
};

}
}
