#pragma once

#include "../controller.h"

namespace smlt {
namespace controllers {

class RigidBody;
class RigidBodySimulation;

class HoverShip:
    public ControllerWithInput,
    public Managed<HoverShip> {

public:
    const std::string name() const {
        return "Hover Ship";
    }

    void set_hover_height(float h);
    void set_hover_force(float f);
    void set_speed(float s);
    void set_turn_speed(float s);

    void update(float dt);
    void fixed_update(float step);

private:
    void on_controller_first_update(Controllable* owner);

    RigidBody* body_ = nullptr;
    RigidBodySimulation* simulation_ = nullptr;

    float hover_height_ = 3.5f;
    float hover_force_ = 64.0f;
    float turn_speed_ = 5.0f;
    float speed_ = 90.0f;

    float power_input_ = 0.0f;
    float turn_input_ = 0.0f;
};

}
}
