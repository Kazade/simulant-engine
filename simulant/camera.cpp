//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "actor.h"
#include "camera.h"
#include "stage.h"
#include "window_base.h"

namespace smlt {

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

void CameraProxy::follow(ActorID actor, CameraFollowMode mode, const smlt::Vec3& offset, float lag_in_seconds) {
    following_actor_ = actor;
    following_offset_ = offset;
    following_lag_ = lag_in_seconds;
    following_mode_ = mode;

    _update_following(1.0);
}

void CameraProxy::_update_following(double dt) {
    if(following_actor_ && stage->has_actor(following_actor_)) {
        float t = (fabs(following_lag_) < kmEpsilon) ? 1.0 : dt * (1.0 / following_lag_);

        auto actor = stage->actor(following_actor_);

        Vec3 avatar_position = actor->absolute_position();
        Quaternion avatar_rotation = actor->absolute_rotation();
        Quaternion initial_rotation = absolute_rotation();
        Vec3 initial_position = absolute_position();

        Vec3 destination_position;
        Quaternion destination_rotation;

        if(following_mode_ == CAMERA_FOLLOW_MODE_DIRECT) {
            destination_position = avatar_position + following_offset_.rotated_by(avatar_rotation);
            destination_rotation = calc_look_at_rotation(avatar_position);
        } else if(following_mode_ == CAMERA_FOLLOW_MODE_THIRD_PERSON) {
            float yaw = kmQuaternionGetYaw(&avatar_rotation);
            kmQuaternionRotationPitchYawRoll(&destination_rotation, 0, yaw, 0);
            destination_position = following_offset_.rotated_by(destination_rotation) + avatar_position;
        } else {
            throw std::logic_error("Unknown camera follow mode");
        }

        set_absolute_position(initial_position.lerp(destination_position, t));
        set_absolute_rotation(initial_rotation.slerp(destination_rotation, t));
    } else {
        //The actor was destroyed, so reset
        following_actor_ = ActorID();
    }
}

void CameraProxy::post_fixed_update(double dt) {
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

void Camera::set_transform(const smlt::Mat4& transform) {
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
        throw std::logic_error("Passed a nullptr as a camera's window");
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
