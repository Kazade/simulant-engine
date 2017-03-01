#include "camera_proxy.h"
#include "../stage.h"
#include "../camera.h"
#include "actor.h"

namespace smlt {


CameraProxy::CameraProxy(CameraID camera_id, Stage *stage):
    StageNode(stage),
    generic::Identifiable<CameraID>(camera_id) {

    assert(stage);

    //Set the camera's proxy to this
    stage->window->camera(camera_id)->set_proxy(this);
}

CameraProxy::~CameraProxy() {
    auto stage = get_stage();

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

        move_to(initial_position.lerp(destination_position, t));
        rotate_to(initial_rotation.slerp(destination_rotation, t));
    } else {
        //The actor was destroyed, so reset
        following_actor_ = ActorID();
    }
}

void CameraProxy::late_update(double dt) {
    StageNode::late_update(dt);

    _update_following(dt);
}

Frustum& CameraProxy::frustum() {
    return camera()->frustum();
}

smlt::optional<Vec3> CameraProxy::project_point(const RenderTarget& target, const Viewport &viewport, const kmVec3& point) {
    return camera()->project_point(target, viewport, point);
}

void CameraProxy::update(double step) {
    StageNode::update(step);

    // Update the associated camera with this transformation
    camera()->set_transform(absolute_transformation());
}

}
