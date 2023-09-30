#include "fly_controller.h"
#include "../scenes/scene.h"

namespace smlt {

void FlyController::on_late_update(float dt) {
    auto& input = scene->input;

    transform->translate(transform->forward() * (input->axis_value("Vertical") * speed_ * dt));
    transform->rotate(smlt::Vec3::POSITIVE_Y, Degrees(input->axis_value("Horizontal") * -50.0f * dt));
    transform->rotate(smlt::Vec3::POSITIVE_Y, Degrees(input->axis_value("MouseX") * -50.0f * dt));
    transform->rotate(transform->right(), Degrees(input->axis_value("MouseY") * -50.0f * dt));
}

}
