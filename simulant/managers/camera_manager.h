#pragma once

#include "../nodes/camera.h"

namespace smlt {

template<typename T, typename IDType, typename ...Subtypes>
class ManualManager;

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

protected:
    void clean_up();

private:
    Stage* stage_;

    typedef ManualManager<Camera, CameraID> Manager;
    std::shared_ptr<Manager> cameras_;
};

}
