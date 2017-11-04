#include "airplane.h"
#include "../physics/rigid_body.h"
#include "../../window.h"
#include "../../input/input_axis.h"

namespace smlt {
namespace controllers {

Airplane::Airplane(Controllable *owner, Window *window):
    ControllerWithInput(window->input.get()),
    owner_(owner),
    window_(window) {
}

void Airplane::fixed_update(float step) {
    RigidBody* rigidbody = owner_->controller<RigidBody>();
    if(!rigidbody) {
        return;
    }

    rigidbody->set_angular_damping(0.6);
    rigidbody->set_linear_damping(0.6);

    rigidbody->add_relative_torque(Vec3(0, -turn_speed_ * step * input->axis_value("Horizontal"), 0));
    rigidbody->add_relative_torque(Vec3(turn_speed_ * step * input->axis_value("Vertical"), 0, 0));
    rigidbody->add_relative_torque(Vec3(turn_speed_ * step * input->axis_value("Roll"), 0, 0));

    throttle_ += throttle_speed_ * step * input->axis_value("Fire1");

    rigidbody->add_relative_force(Vec3(0, 0, -throttle_ * step));
}

void Airplane::on_controller_added(Controllable *controllable) {
    if(!input->axis_count("Roll")) {
        auto axis = input->new_axis("Roll");
        axis->set_positive_keyboard_key(KEYBOARD_CODE_E);
        axis->set_negative_keyboard_key(KEYBOARD_CODE_Q);
    }
}

}
}
