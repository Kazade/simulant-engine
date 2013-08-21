#include "actor.h"
#include "camera.h"
#include "scene.h"
#include "stage.h"
#include "window_base.h"
#include "kazbase/unicode.h"

namespace kglt {

CameraProxy::CameraProxy(Stage *stage, CameraID camera_id):
    Object(stage),
    generic::Identifiable<CameraID>(camera_id) {

    assert(stage);

    //Set the camera's proxy to this
    stage->scene().camera(camera_id).set_proxy(this);
}

CameraProxy::~CameraProxy() {
    //Set the camera's proxy to null
    if(&stage().scene().camera(id()).proxy() == this) {
        stage().scene().camera(id()).set_proxy(nullptr);
    }
}

void CameraProxy::destroy() {
    destroy_children();
    stage().evict_camera(id());
}

Camera& CameraProxy::camera() {
    return stage().scene().camera(id());
}

void CameraProxy::follow(ActorID actor, const kglt::Vec3& offset, float lag_in_seconds) {
    following_actor_ = actor;
    following_offset_ = offset;
    following_lag_ = lag_in_seconds;

    update_following(1.0);
}

void CameraProxy::update_following(double dt) {
    if(following_actor_ && stage().has_actor(following_actor_)) {
        Quaternion actor_rotation = stage().actor(following_actor_)->absolute_rotation();
        Vec3 actor_position = stage().actor(following_actor_)->absolute_position();

        Vec3 actor_forward;
        kmQuaternionGetForwardVec3RH(&actor_forward, &actor_rotation);

        Quaternion initial_rotation = absolute_rotation();

        float t = ((following_lag_ == 0) ? 1.0 : dt * (1.0 / following_lag_));

        set_absolute_rotation(
            initial_rotation.slerp(actor_rotation, t)
        );

        Vec3 rotated_offset = following_offset_.rotate(absolute_rotation());
        set_absolute_position(rotated_offset + actor_position);

        update_from_parent();
    } else {
        //The actor was destroyed, so reset
        following_actor_ = ActorID();
    }
}

void CameraProxy::do_update(double dt) {
    update_following(dt);
}


Camera::Camera(Scene *scene, CameraID id):
    generic::Identifiable<CameraID>(id),
    scene_(scene),
    proxy_(nullptr) {

    kmMat4Identity(&transform_);
    kmMat4Identity(&projection_matrix_); //Initialize the projection matrix
    kmMat4Identity(&view_matrix_);

    set_perspective_projection(45.0, 16.0 / 9.0);
}

void Camera::set_transform(const kglt::Mat4& transform) {
    transform_ = transform;
    update_frustum();
}

void Camera::update_frustum() {
    //Recalculate the view matrix
    kmMat4Inverse(&view_matrix_, &transform_);

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
