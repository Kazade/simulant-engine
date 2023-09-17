#include "fly_controller.h"
#include "../scenes/scene.h"

namespace smlt {

void FlyController::on_late_update(float dt) {
    auto& input = scene->input;

    move_forward_by(input->axis_value("Vertical") * speed_ * dt);
    rotate_global_y_by(Degrees(input->axis_value("Horizontal") * -50.0f * dt));
    rotate_global_y_by(Degrees(input->axis_value("MouseX") * -50.0f * dt));
    rotate_x_by(Degrees(input->axis_value("MouseY") * -50.0f * dt));
}

}
