#include "airplane.h"
#include "../physics/rigid_body.h"
#include "../../window.h"

namespace smlt {
namespace controllers {

void Airplane::on_key_down(const KeyEvent& evt) {
    switch(evt.keyboard_code) {
        case KEYBOARD_CODE_LEFT:
            yaw_left_ = true;
        break;
        case KEYBOARD_CODE_RIGHT:
            yaw_right_ = true;
        break;
        case KEYBOARD_CODE_UP:
            pitch_up_ = true;
        break;
        case KEYBOARD_CODE_DOWN:
            pitch_down_ = true;
        break;
        case KEYBOARD_CODE_W:
            accelerating_ = true;
        break;
        case KEYBOARD_CODE_A:
            roll_left_ = true;
        break;
        case KEYBOARD_CODE_D:
            roll_right_ = true;
        break;
        default:
            break;
    }
}

void Airplane::on_key_up(const KeyEvent& evt) {
    switch(evt.keyboard_code) {
        case KEYBOARD_CODE_LEFT:
            yaw_left_ = false;
        break;
        case KEYBOARD_CODE_RIGHT:
            yaw_right_ = false;
        break;
        case KEYBOARD_CODE_UP:
            pitch_up_ = false;
        break;
        case KEYBOARD_CODE_DOWN:
            pitch_down_ = false;
        break;
        case KEYBOARD_CODE_W:
            accelerating_ = false;
        break;
        case KEYBOARD_CODE_A:
            roll_left_ = false;
        break;
        case KEYBOARD_CODE_D:
            roll_right_ = false;
        break;
        default:
            break;
    }
}

void Airplane::fixed_update(float step) {
    RigidBody* rigidbody = owner_->controller<RigidBody>();
    if(!rigidbody) {
        return;
    }

    rigidbody->set_angular_damping(0.6);
    rigidbody->set_linear_damping(0.6);

    if(yaw_left_) {
        rigidbody->add_relative_torque(Vec3(0, turn_speed_ * step, 0));
    }

    if(yaw_right_) {
        rigidbody->add_relative_torque(Vec3(0, -turn_speed_ * step, 0));
    }

    if(pitch_up_) {
        rigidbody->add_relative_torque(Vec3(-turn_speed_ * step, 0, 0));
    }

    if(pitch_down_) {
        rigidbody->add_relative_torque(Vec3(turn_speed_ * step, 0, 0));
    }

    if(roll_left_) {
        rigidbody->add_relative_torque(Vec3(0, 0, turn_speed_ * step));
    }

    if(roll_right_) {
        rigidbody->add_relative_torque(Vec3(0, 0, -turn_speed_ * step));
    }

    if(accelerating_) {
        throttle_ += throttle_speed_ * step;
    }

    rigidbody->add_relative_force(Vec3(0, 0, -throttle_ * step));
}

void Airplane::on_controller_added(Controllable *controllable) {
    window_->register_event_listener(this);
}

void Airplane::on_controller_removed(Controllable *controllable) {
    window_->unregister_event_listener(this);
}

}
}
