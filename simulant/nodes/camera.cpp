#include "camera.h"
#include "actor.h"

#include "../stage.h"
#include "../window.h"
#include "../viewport.h"
#include "../application.h"

namespace smlt {


Camera::Camera(Stage *stage, SoundDriver* sound_driver):
    TypedDestroyableObject<Camera, Stage>(stage),
    ContainerNode(stage, STAGE_NODE_TYPE_CAMERA),
    AudioSource(stage, this, sound_driver){

    assert(stage);

    set_perspective_projection(smlt::Degrees(45.0f), get_app()->window->aspect_ratio());
}

Camera::~Camera() {

}

void Camera::update(float dt) {
    StageNode::update(dt);
}

void Camera::update_transformation_from_parent() {
    StageNode::update_transformation_from_parent();
    transform_ = absolute_transformation();
    update_frustum();
}

void Camera::update_frustum() {
    //Recalculate the view matrix
    view_matrix_ = Mat4::as_look_at(absolute_position(), absolute_position() + forward(), smlt::Vec3::POSITIVE_Y);

    Mat4 mvp = projection_matrix_ * view_matrix_;

    frustum_.build(&mvp); //Update the frustum for this camera
}

void Camera::set_perspective_projection(const Degrees& fov, float aspect, float near, float far) {
    projection_matrix_ = Mat4::as_projection(fov, aspect, near, far);
    update_frustum();
}

void Camera::set_orthographic_projection(float left, float right, float bottom, float top, float near, float far) {
    projection_matrix_ = Mat4::as_orthographic(left, right, bottom, top, near, far);
    update_frustum();
}

float Camera::set_orthographic_projection_from_height(float desired_height_in_units, float ratio) {
    float width = desired_height_in_units * ratio;
    set_orthographic_projection(-width / 2.0f, width / 2.0f, -desired_height_in_units / 2.0f, desired_height_in_units / 2.0f, -10.0f, 10.0f);
    return width;
}

smlt::optional<Vec3> Camera::project_point(const RenderTarget &target, const Viewport &viewport, const Vec3& point) const {
    Vec4 in(point, 1.0);
    Vec4 out = view_matrix() * in;
    in = projection_matrix() * out;

    if(in.w == 0.0f) {
        return smlt::optional<Vec3>();
    }

    in.x /= in.w;
    in.y /= in.w;
    in.z /= in.w;

    in.x = in.x * 0.5f + 0.5f;
    in.y = in.y * 0.5f + 0.5f;
    in.z = in.z * 0.5f + 0.5f;

    float vp_width = viewport.width_in_pixels(target);
    float vp_height = viewport.height_in_pixels(target);
    float vp_xoffset = target.width() * viewport.x();
    float vp_yoffset = target.height() * viewport.y();

    return Vec3(
        in.x * vp_width + vp_xoffset,
        in.y * vp_height + vp_yoffset,
        in.z
    );
}

smlt::optional<Vec3> Camera::unproject_point(const RenderTarget& target, const Viewport& viewport, const Vec3& win) {
    /*
     * WARNING: This function is untested (FIXME) !!!
     */

    Mat4 A, m;
    Vec4 in;

    A = projection_matrix() * view_matrix();
    m = A.inversed();

    float vx = float(target.width()) * viewport.x();
    float vy = float(target.height()) * viewport.y();
    float vw = (float) viewport.width_in_pixels(target);
    float vh = (float) viewport.height_in_pixels(target);

    in.x = (win.x - vx) / vw * 2.0f - 1.0f;
    in.y = (win.y - vy) / vh * 2.0f - 1.0f;
    in.z = 2.0f * win.z - 1.0f;
    in.w = 1.0f;

    Vec4 out = m * in;

    if(out.w == 0.0f) {
        return smlt::optional<Vec3>();
    }

    out.w = 1.0f / out.w;

    Vec3 ret;

    ret.x = out.x * out.w;
    ret.y = out.y * out.w;
    ret.z = out.z * out.w;

    return smlt::optional<Vec3>(std::move(ret));
}

}
