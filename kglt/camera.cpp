
#include "camera.h"
#include "scene.h"
#include "window_base.h"

namespace kglt {

Camera::Camera(SubScene *subscene, CameraID id):
    Object(subscene),    
    generic::Identifiable<CameraID>(id),
    Source(*subscene) {

    kmMat4Identity(&projection_matrix_); //Initialize the projection matrix
    kmMat4Identity(&modelview_matrix_);

    set_perspective_projection(45.0, 16.0 / 9.0);
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

void Camera::destroy() {
    subscene().delete_camera(id());
}

void Camera::follow(EntityID entity, const kglt::Vec3& offset) {
    following_entity_ = entity;
    following_offset_ = offset;
}

void Camera::do_update(double dt) {
    if(following_entity_) {
        kmQuaternion new_rotation;
        kmQuaternion entity_rotation = subscene().entity(following_entity_).absolute_rotation();
        kmQuaternionSlerp(&new_rotation, &rotation_, &entity_rotation, 1.0 * dt);

        kmVec3 rotated_offset;
        kmVec3 entity_position = subscene().entity(following_entity_).absolute_position();
        kmMat4 new_rotation_matrix;
        kmMat4RotationQuaternion(&new_rotation_matrix, &new_rotation);
        kmVec3MultiplyMat4(&rotated_offset, &following_offset_, &new_rotation_matrix);

        kmVec3Add(&rotated_offset, &rotated_offset, &entity_position);

        kmVec3Assign(&position_, &rotated_offset);
        kmQuaternionAssign(&rotation_, &new_rotation);
        update_from_parent();
    }
}

}
