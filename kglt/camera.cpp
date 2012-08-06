
#include "camera.h"
#include "scene.h"
#include "window_base.h"

namespace kglt {

Camera::Camera(Scene *scene):
    Object(scene) {

    kmQuaternionRotationYawPitchRoll(&rotation(), 180.0, 0.0, 0.0);
    kmQuaternionNormalize(&rotation(), &rotation());

    kmMat4Identity(&projection_matrix_); //Initialize the projection matrix
}

void Camera::set_perspective_projection(double fov, double aspect, double near, double far) {
    kmMat4PerspectiveProjection(&projection_matrix_, fov, aspect, near, far);
    update_frustum();
}

void Camera::set_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    kmMat4OrthographicProjection(&projection_matrix_, left, right, bottom, top, near, far);
    update_frustum();
}

double Camera::set_orthographic_projection_from_height(double desired_height_in_units, double ratio) {
    double width = desired_height_in_units * ratio;
    set_orthographic_projection(-width / 2.0, width / 2.0, -desired_height_in_units / 2.0, desired_height_in_units / 2.0, -10.0, 10.0);
    return width;
}

}
