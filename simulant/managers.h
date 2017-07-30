/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MANAGERS_H
#define MANAGERS_H

#include "generic/generic_tree.h"
#include "generic/manager.h"
#include "generic/manual_manager.h"
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

    void update(float dt) override;

    Property<BackgroundManager, WindowBase> window = { this, &BackgroundManager::window_ };
private:
    WindowBase* window_;
};

class CameraManager {
public:
    CameraManager(Stage* stage);

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
    Stage* stage_;

    generic::ManualManager<Camera, CameraID> cameras_;
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
    void fixed_update(float dt) override;
    void update(float dt) override;
    void late_update(float dt) override;

    void delete_all_stages();
private:
    WindowBase* window_ = nullptr;
    void print_tree(StageNode* node, uint32_t& level);
};

}

#endif // MANAGERS_H
