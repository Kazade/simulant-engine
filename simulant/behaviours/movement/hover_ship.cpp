
#include "hover_ship.h"

#include "../../core.h"
#include "../physics/rigid_body.h"
#include "../physics/simulation.h"
#include "../../macros.h"

namespace smlt {
namespace behaviours {

HoverShip::HoverShip(Core* core):
    BehaviourWithInput(core->input.get()) {

}

void HoverShip::on_behaviour_first_update(Organism* owner) {
    body_ = owner->behaviour<RigidBody>();
    assert(body_ && "No RigidBody behaviour attached");
    simulation_ = body_->simulation;
}

void HoverShip::set_hover_height(float h) {
    hover_height_ = h;
}

void HoverShip::set_hover_force(float f) {
    hover_force_ = f;
}

void HoverShip::set_speed(float s) {
    speed_ = s;
}

void HoverShip::set_turn_speed(float s) {
    turn_speed_ = s;
}

void HoverShip::update(float dt) {
    _S_UNUSED(dt);

    power_input_ = input->axis_value("Vertical");
    turn_input_ = input->axis_value("Horizontal");
}

void HoverShip::fixed_update(float step) {
    _S_UNUSED(step);

    float dist = 0.0f;
    auto hit = simulation_->intersect_ray(body_->position(), -body_->up() * hover_height_, &dist);
    if(hit.second) {
        auto prop_height = (hover_height_ - dist) / hover_height_;
        auto force = smlt::Vec3::POSITIVE_Y * prop_height * hover_force_;
        body_->add_force(force);
    }

    body_->add_relative_force(smlt::Vec3(0.0f, 0.0f, -power_input_ * speed_));
    body_->add_relative_torque(smlt::Vec3(0.0f, -turn_input_ * turn_speed_, 0.0f));
}


}
}
