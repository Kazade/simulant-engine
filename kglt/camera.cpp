#include "actor.h"
#include "camera.h"
#include "stage.h"
#include "window_base.h"
#include "kazbase/unicode.h"

namespace kglt {

CameraProxy::CameraProxy(CameraID camera_id, Stage *stage):
    ParentSetterMixin<MoveableObject>(stage),
    generic::Identifiable<CameraID>(camera_id) {

    assert(stage);

    //Set the camera's proxy to this
    stage->window->camera(camera_id)->set_proxy(this);
}

CameraProxy::~CameraProxy() {
    //Set the camera's proxy to null
    if(!stage->window->is_shutting_down() &&
        stage->window->has_camera(id())) {

        if(&stage->window->camera(id())->proxy() == this) {
            stage->window->camera(id())->set_proxy(nullptr);
        }
    }
}

void CameraProxy::ask_owner_for_destruction() {
    stage->evict_camera(id());
}

void CameraProxy::set_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    camera()->set_orthographic_projection(left, right, bottom, top, near, far);
}

CameraPtr CameraProxy::camera() {
    return stage->window->camera(id());
}

void CameraProxy::follow(ActorID actor, CameraFollowMode mode, const kglt::Vec3& offset, float lag_in_seconds) {
    following_actor_ = actor;
    following_offset_ = offset;
    following_lag_ = lag_in_seconds;
    following_mode_ = mode;

    _update_following(1.0);
}

void CameraProxy::_update_following(double dt) {
    if(following_actor_ && stage->has_actor(following_actor_)) {
        float t = ((following_lag_ == 0) ? 1.0 : dt * (1.0 / following_lag_));

        auto actor = stage->actor(following_actor_);

        Vec3 avatar_position = actor->absolute_position();
        Quaternion avatar_rotation = actor->absolute_rotation();
        Quaternion initial_rotation = absolute_rotation();
        Vec3 initial_position = absolute_position();

        Vec3 destination_position;
        Quaternion destination_rotation;

        if(following_mode_ == CAMERA_FOLLOW_MODE_DIRECT) {
            destination_position = following_offset_.rotated_by(avatar_rotation) + avatar_position;
            destination_rotation = avatar_rotation;
        } else if(following_mode_ == CAMERA_FOLLOW_MODE_THIRD_PERSON) {
            float yaw = kmQuaternionGetYaw(&avatar_rotation);
            kmQuaternionRotationPitchYawRoll(&destination_rotation, 0, yaw, 0);
            destination_position = following_offset_.rotated_by(destination_rotation) + avatar_position;
        } else {
            throw ValueError("Unknown camera follow mode");
        }

        // If we're close, just position directly
        if((destination_position - initial_position).length_squared() < 0.1f) {
            set_absolute_position(destination_position);
        } else {
            set_absolute_position(initial_position.lerp(destination_position, t));
        }

        set_absolute_rotation(initial_rotation.slerp(destination_rotation, t));

        update_from_parent();
    } else {
        //The actor was destroyed, so reset
        following_actor_ = ActorID();
    }
}

void CameraProxy::do_update(double dt) {
    _update_following(dt);
}

Frustum& CameraProxy::frustum() {
    return camera()->frustum();
}

kmVec3 CameraProxy::project_point(const RenderTarget& target, const Viewport &viewport, const kmVec3& point) {
    return camera()->project_point(target, viewport, point);
}

Camera::Camera(CameraID id, WindowBase *window):
    generic::Identifiable<CameraID>(id),
    window_(window),
    proxy_(nullptr) {

    kmMat4Identity(&transform_);
    kmMat4Identity(&projection_matrix_); //Initialize the projection matrix
    kmMat4Identity(&view_matrix_);

    set_perspective_projection(45.0, float(window->width()) / float(window->height()));
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

kmVec3 Camera::project_point(const RenderTarget &target, const Viewport &viewport, const kmVec3& point) {
    if(!window_) {
        throw LogicError("Passed a nullptr as a camera's window");
    }

    kmVec3 tmp;
    kmVec3Fill(&tmp, point.x, point.y, point.z);

    kmVec3 result;

    kmVec3MultiplyMat4(&tmp, &tmp, &view_matrix());
    kmVec3MultiplyMat4(&tmp, &tmp, &projection_matrix());

    tmp.x /= tmp.z;
    tmp.y /= tmp.z;

    float vp_width = viewport.width_in_pixels(target);
    float vp_height = viewport.height_in_pixels(target);

    result.x = (tmp.x + 1) * vp_width / 2.0;
    result.y = (tmp.y + 1) * vp_height / 2.0;

    return result;
}

}
