#include "camera.h"

#include "../stage.h"
#include "actor.h"

namespace smlt {


Camera::Camera(CameraID camera_id, Stage *stage):
    ContainerNode(stage),
    generic::Identifiable<CameraID>(camera_id) {

    assert(stage);

    set_perspective_projection(smlt::Degrees(45.0f), stage->window->aspect_ratio());
}

Camera::~Camera() {

}

void Camera::ask_owner_for_destruction() {
    stage->delete_camera(id());
}

void Camera::update_transformation_from_parent() {
    StageNode::update_transformation_from_parent();
    transform_ = absolute_transformation();
    update_frustum();
}

void Camera::update_frustum() {
    //Recalculate the view matrix
    view_matrix_ = transform_.inversed();

    Mat4 mvp = projection_matrix_ * view_matrix_;

    frustum_.build(&mvp); //Update the frustum for this camera
}

void Camera::set_perspective_projection(const Degrees& fov, double aspect, double near, double far) {
    projection_matrix_ = Mat4::as_projection(fov, aspect, near, far);
    update_frustum();
}

void Camera::set_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    projection_matrix_ = Mat4::as_orthographic(left, right, bottom, top, near, far);
    update_frustum();
}

double Camera::set_orthographic_projection_from_height(double desired_height_in_units, double ratio) {
    double width = desired_height_in_units * ratio;
    set_orthographic_projection(-width / 2.0, width / 2.0, -desired_height_in_units / 2.0, desired_height_in_units / 2.0, -10.0, 10.0);
    return width;
}

smlt::optional<Vec3> Camera::project_point(const RenderTarget &target, const Viewport &viewport, const Vec3& point) const {
    Vec4 in(point, 1.0);
    Vec4 out = view_matrix() * in;
    in = projection_matrix() * out;

    if(in.w == 0.0) {
        return smlt::optional<Vec3>();
    }

    in.x /= in.w;
    in.y /= in.w;
    in.z /= in.w;

    in.x = in.x * 0.5 + 0.5;
    in.y = in.y * 0.5 + 0.5;
    in.z = in.z * 0.5 + 0.5;

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

    in.x = (win.x - vx) / vw * 2.0 - 1.0;
    in.y = (win.y - vy) / vh * 2.0 - 1.0;
    in.z = 2.0 * win.z - 1.0;
    in.w = 1.0;

    Vec4 out = m * in;

    if(out.w == 0.0f) {
        return smlt::optional<Vec3>();
    }

    out.w = 1.0 / out.w;

    Vec3 ret;

    ret.x = out.x * out.w;
    ret.y = out.y * out.y;
    ret.z = out.z * out.z;

    return smlt::optional<Vec3>(std::move(ret));
}

}
