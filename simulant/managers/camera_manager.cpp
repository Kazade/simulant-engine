#include "camera_manager.h"
#include "../stage.h"
#include "../window.h"

namespace smlt {

//=============== START CAMERAS ============

CameraManager::CameraManager(Stage *stage):
    stage_(stage) {

}

CameraPtr CameraManager::new_camera() {
    auto new_camera = cameras_.make(this->stage_);
    new_camera->set_parent(stage_);

    return new_camera;
}

CameraPtr CameraManager::new_camera_with_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    /*
     *  Instantiates a camera with an orthographic projection. If both left and right are zero then they default to 0 and window.width()
     *  respectively. If top and bottom are zero, then they default to window.height() and 0 respectively. So top left is 0,0
     */
    auto new_cam = new_camera();

    if(!left && !right) {
        right = stage_->window->width();
    }

    if(!bottom && !top) {
        top = stage_->window->height();
    }

    new_cam->set_orthographic_projection(left, right, bottom, top, near, far);

    return new_cam;
}

CameraPtr CameraManager::new_camera_for_viewport(const Viewport& vp) {
    float x, y, width, height;
    calculate_ratios_from_viewport(vp.type(), x, y, width, height);

    auto camera = new_camera();
    camera->set_perspective_projection(Degrees(45.0), width / height);

    return camera;
}

CameraPtr CameraManager::new_camera_for_ui() {
    return new_camera_with_orthographic_projection(0, stage_->window->width(), 0, stage_->window->height(), -1, 1);
}

CameraPtr CameraManager::camera(CameraID c) {
    return cameras_.get(c);
}

void CameraManager::delete_camera(CameraID cid) {
    cameras_.destroy(cid);
}

uint32_t CameraManager::camera_count() const {
    return cameras_.size();
}

bool CameraManager::has_camera(CameraID id) const {
    return cameras_.contains(id);
}

void CameraManager::delete_all_cameras() {
    cameras_.clear();
}

void CameraManager::clean_up() {
    cameras_.clean_up();
}

//============== END CAMERAS ================

}
