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
