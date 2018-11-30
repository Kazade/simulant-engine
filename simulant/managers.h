/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
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
    StageManager(Window* window);

    StagePtr new_stage(AvailablePartitioner partitioner=PARTITIONER_HASH);
    StagePtr stage(StageID s);
    StagePtr delete_stage(StageID s);
    std::size_t stage_count() const;
    bool has_stage(StageID stage_id) const;

    void print_tree();
    void fixed_update(float dt) override;
    void update(float dt) override;
    void late_update(float dt) override;

    void delete_all_stages();
private:
    Window* window_ = nullptr;
    void print_tree(StageNode* node, uint32_t& level);
};

}

#endif // MANAGERS_H
