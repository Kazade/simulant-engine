#include "input.h"
#include "input_manager.h"
#include "../input_controller.h"

namespace smlt {

InputManager::InputManager(InputController *controller):
    controller_(controller) {

    auto horizontal = new_axis("Horizontal");
    horizontal->set_positive_keyboard_key(KEYBOARD_CODE_D);
    horizontal->set_negative_keyboard_key(KEYBOARD_CODE_A);

    auto horizontal_alt = new_axis("Horizontal");
    horizontal_alt->set_positive_keyboard_key(KEYBOARD_CODE_RIGHT);
    horizontal_alt->set_negative_keyboard_key(KEYBOARD_CODE_LEFT);

    auto vertical = new_axis("Vertical");
    vertical->set_positive_keyboard_key(KEYBOARD_CODE_W);
    vertical->set_negative_keyboard_key(KEYBOARD_CODE_S);

    auto vertical_alt = new_axis("Vertical");
    vertical_alt->set_positive_keyboard_key(KEYBOARD_CODE_UP);
    vertical_alt->set_negative_keyboard_key(KEYBOARD_CODE_DOWN);
}

InputAxis* InputManager::new_axis(const std::string& name) {
    auto axis = InputAxis::create(name);
    axises_.push_back(axis);
    return axis.get();
}

AxisList InputManager::axises(const std::string& name) const {
    AxisList result;
    for(auto& axis: axises_) {
        result.push_back(axis.get());
    }
    return result;
}

void InputManager::each_axis(EachAxisCallback callback) {
    for(auto& axis: axises_) {
        callback(axis.get());
    }
}

void InputManager::delete_axises(const std::string& name) {
    axises_.erase(
        std::remove_if(axises_.begin(), axises_.end(),
            [&name](InputAxis::ptr axis) -> bool {
                return axis->name() == name;
            }
        ), axises_.end()
    );
}

void InputManager::delete_axis(InputAxis* axis) {
    axises_.erase(
        std::remove_if(axises_.begin(), axises_.end(),
            [axis](InputAxis::ptr ax) -> bool {
                return ax.get() == axis;
            }
        ), axises_.end()
    );
}

float sgn(float v) {
    if(v > 0) { return 1.0f; }
    else { return -1.0f; }
}

void InputManager::update(float dt) {
    for(auto axis: axises_) {
        float new_value = 0.0f;

        if(axis->type() == AXIS_TYPE_KEY_OR_MOUSE_BUTTON) {
            auto pkey = axis->positive_keyboard_key();
            auto nkey = axis->negative_keyboard_key();

            bool positive_pressed = false;
            bool negative_pressed = false;

            auto process_keyboard = [this, pkey, nkey, &positive_pressed, &negative_pressed](KeyboardID id) {
                if(pkey != KEYBOARD_CODE_NONE && controller_->keyboard_key_state(id, pkey)) {
                    positive_pressed = true;
                }

                if(nkey != KEYBOARD_CODE_NONE && controller_->keyboard_key_state(id, pkey)) {
                    negative_pressed = true;
                }
            };

            // If the user requested input from all keyboards, do that
            if(axis->keyboard_source() == ALL_KEYBOARDS) {
                for(std::size_t i = 0; i < controller_->keyboard_count(); ++i) {
                    KeyboardID id = (KeyboardID) i;
                    process_keyboard(id);
                }
            } else {
                // Otherwise just check the one they asked for
                KeyboardID id = axis->keyboard_source();
                process_keyboard(id);
            }

            // If either positive or negative were pressed, adjust the value
            if(positive_pressed) new_value = 1.0f;
            if(negative_pressed) new_value = -1.0f;

            // If neither were pressed, then apply the return speed (making sure we don't pass zero)
            if(!positive_pressed && !negative_pressed) {
                auto sign = sgn(axis->value());
                new_value = std::max(0.0f, (abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
            }

            axis->value_ = new_value;
        }
    }
}


float InputManager::axis_value(const std::string& name) const {
    auto f = 0.0f;
    for(auto axis: axises(name)) {
        auto v = axis->value();

        // Return the result with the greatest overall value (positive or negative)
        if(fabs(v) > fabs(f)) {
            f = v;
        }
    }

    return f;
}


}
