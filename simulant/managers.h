#ifndef MANAGERS_H
#define MANAGERS_H

#include "generic/generic_tree.h"
#include "generic/manager.h"
#include "generic/property.h"
#include "types.h"
#include "interfaces.h"
#include "interfaces/updateable.h"

namespace smlt {

class StageNode;

class BackgroundManager:
    public generic::TemplatedManager<Background, BackgroundID>,
    public virtual Updateable {

public:
    BackgroundManager(WindowBase* window);
    ~BackgroundManager();

    BackgroundID new_background();
    BackgroundID new_background_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0);
    BackgroundPtr background(BackgroundID bid);
    bool has_background(BackgroundID bid) const;
    void delete_background(BackgroundID bid);
    uint32_t background_count() const;

    void update(double dt) override;

    Property<BackgroundManager, WindowBase> window = { this, &BackgroundManager::window_ };
private:
    WindowBase* window_;
};

class CameraManager:
    public generic::TemplatedManager<Camera, CameraID> {

public:
    CameraManager(WindowBase* window);

    CameraID new_camera();
    CameraID new_camera_with_orthographic_projection(double left=0, double right=0, double bottom=0, double top=0, double near=-1.0, double far=1.0);
    CameraID new_camera_for_ui();
    CameraID new_camera_for_viewport(const Viewport& vp);
    CameraPtr camera(CameraID c);
    void delete_camera(CameraID cid);
    uint32_t camera_count() const;
    const bool has_camera(CameraID id) const;
    void delete_all_cameras();

private:
    WindowBase* window_;
};

typedef sig::signal<void (StageID)> StageAddedSignal;
typedef sig::signal<void (StageID)> StageRemovedSignal;

class StageManager:
    public BaseStageManager,
    public virtual Updateable {

    DEFINE_SIGNAL(StageAddedSignal, signal_stage_added);
    DEFINE_SIGNAL(StageRemovedSignal, signal_stage_removed);

public:
    StageManager(WindowBase* window);

    StageID new_stage(AvailablePartitioner partitioner=PARTITIONER_HASH);
    StagePtr stage(StageID s);
    void delete_stage(StageID s);
    uint32_t stage_count() const;
    bool has_stage(StageID stage_id) const;

    void print_tree();
    void fixed_update(double dt);
    void update(double dt) override;

    void pre_update(double dt);
    void post_update(double dt);
    void pre_fixed_update(double step);
    void post_fixed_update(double step);

    void delete_all_stages();
private:
    WindowBase* window_ = nullptr;
    void print_tree(StageNode* node, uint32_t& level);
};

class OverlayManager:
    public generic::TemplatedManager<Overlay, OverlayID> {

public:
    OverlayManager(WindowBase* window);

    OverlayID new_overlay();
    OverlayID new_overlay_from_file(const unicode& rml_file);

    OverlayPtr overlay(OverlayID s);
    void delete_overlay(OverlayID s);
    uint32_t overlay_count() const;

    bool has_overlay(OverlayID overlay) const;

    void delete_all_overlays();
private:
    WindowBase* window_;

};

}

#endif // MANAGERS_H
