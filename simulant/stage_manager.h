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

#pragma once

#include "generic/generic_tree.h"
#include "generic/property.h"
#include "generic/manual_manager.h"
#include "interfaces.h"
#include "interfaces/updateable.h"
#include "types.h"

namespace smlt {

class StageNode;

typedef sig::signal<void (StageID)> StageAddedSignal;
typedef sig::signal<void (StageID)> StageRemovedSignal;


class StageManager:
    public virtual Updateable {

    DEFINE_SIGNAL(StageAddedSignal, signal_stage_added);
    DEFINE_SIGNAL(StageRemovedSignal, signal_stage_removed);

public:
    typedef ManualManager<Stage, StageID>::iterator_pair stage_iterator_pair;

    StageManager(Window* window);

    StagePtr new_stage(AvailablePartitioner partitioner=PARTITIONER_HASH);
    StagePtr stage(StageID s);
    StagePtr destroy_stage(StageID s);
    std::size_t stage_count() const;
    bool has_stage(StageID stage_id) const;

    void print_tree();
    void fixed_update(float dt) override;
    void update(float dt) override;
    void late_update(float dt) override;

    void destroy_all_stages();

    stage_iterator_pair each_stage() {
        return stage_manager_->_each();
    }

    /* Implementation for TypedDestroyableObject (INTERNAL) */
    void destroy_object(Stage* object);
    void destroy_object_immediately(Stage* object);
private:
    Window* window_ = nullptr;
    void print_tree(StageNode* node, uint32_t& level);

protected:
    void clean_up();
    std::shared_ptr<ManualManager<Stage, StageID>> stage_manager_;
};

}
