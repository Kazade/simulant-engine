#pragma once

#include "../nodes/camera.h"
#include "../generic/manual_manager.h"

namespace smlt {

class CameraManager {
public:
    CameraManager(Stage* stage);

    CameraPtr new_camera();
    CameraPtr new_camera_with_orthographic_projection(double left=0, double right=0, double bottom=0, double top=0, double near=-1.0, double far=1.0);
    CameraPtr new_camera_for_ui();
    CameraPtr new_camera_for_viewport(const Viewport& vp);
    CameraPtr camera(CameraID c);
    void delete_camera(CameraID cid);
    uint32_t camera_count() const;
    bool has_camera(CameraID id) const;
    void delete_all_cameras();

private:
    Stage* stage_;

    ManualManager<Camera, CameraID> cameras_;
};

}
