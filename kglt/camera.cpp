#include "entity.h"
#include "camera.h"
#include "scene.h"
#include "stage.h"
#include "window_base.h"
#include "kazbase/unicode.h"

namespace kglt {

Camera::Camera(Scene *scene, CameraID id):
    Object(nullptr),
    generic::Identifiable<CameraID>(id),
    scene_(scene) {

    kmMat4Identity(&projection_matrix_); //Initialize the projection matrix
    kmMat4Identity(&view_matrix_);

    set_perspective_projection(45.0, 16.0 / 9.0);
}

void Camera::update_frustum() {
    //Recalculate the view matrix
    kmMat4 transform = this->absolute_transformation();
    kmMat4Inverse(&view_matrix_, &transform);

    kmMat4 mvp;
    kmMat4Multiply(&mvp, &projection_matrix(), &view_matrix());

    frustum_.build(&mvp); //Update the frustum for this camera
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
    if(!scene_) {
        throw LogicError("Passes a nullptr for the camera's scene");
    }

    scene_->delete_camera(id());
}

void Camera::follow(EntityRef entity, const kglt::Vec3& offset) {
    following_entity_ = entity;
    following_offset_ = offset;
}

void Camera::do_update(double dt) {
    EntityPtr following_entity = following_entity_.lock();
    if(following_entity) {
        kmQuaternion entity_rotation = following_entity->absolute_rotation();
        kmVec3 entity_position = following_entity->absolute_position();

        kmVec3 entity_forward;
        kmQuaternionGetForwardVec3RH(&entity_forward, &entity_rotation);

        kmQuaternion initial_rotation;
        kmQuaternionAssign(&initial_rotation, &rotation_);
        kmQuaternionSlerp(&rotation_, &initial_rotation, &entity_rotation, dt);

        kmVec3 rotated_offset;
        kmQuaternionMultiplyVec3(&rotated_offset, &rotation_, &following_offset_);

        //kmMat4RotationQuaternion(&new_rotation_matrix, &rotation_);
        //kmVec3MultiplyMat4(&rotated_offset, &following_offset_, &new_rotation_matrix);
        kmVec3Add(&position_, &rotated_offset, &entity_position);

        update_from_parent();
    } else {
        //The entity was destroyed, so reset
        following_entity_ = EntityRef();
    }
}

kmVec3 Camera::project_point(ViewportID vid, const kmVec3& point) {
    if(!scene_) {
        throw LogicError("Passes a nullptr as a camera's Scene");
    }

    kglt::Viewport& viewport = scene_->window().viewport(vid);

    kmVec3 tmp;
    kmVec3Fill(&tmp, point.x, point.y, point.z);

    kmVec3 result;

    kmVec3MultiplyMat4(&tmp, &tmp, &view_matrix());
    kmVec3MultiplyMat4(&tmp, &tmp, &projection_matrix());

    tmp.x /= tmp.z;
    tmp.y /= tmp.z;

    float vp_width = viewport.width();
    float vp_height = viewport.height();

    result.x = (tmp.x + 1) * vp_width / 2.0;
    result.y = (tmp.y + 1) * vp_height / 2.0;

    return result;
}

}
