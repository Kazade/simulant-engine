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
#include "generic/containers/polylist.h"
#include "interfaces.h"
#include "interfaces/updateable.h"
#include "types.h"
#include "managers/window_holder.h"

#include "nodes/stage_node_pool.h"

namespace smlt {

class StageNode;

typedef sig::signal<void (StageID)> StageAddedSignal;
typedef sig::signal<void (StageID)> StageRemovedSignal;

template<typename PoolType, typename IDType, typename T, typename ...Subtypes>
class StageNodeManager;

class StageManager:
    public virtual Updateable {

    DEFINE_SIGNAL(StageAddedSignal, signal_stage_added);
    DEFINE_SIGNAL(StageRemovedSignal, signal_stage_removed);

public:
    struct IteratorPair {
        friend class StageManager;

        std::list<Stage*>::iterator begin();
        std::list<Stage*>::iterator end();

    private:
        IteratorPair(StageManager* owner):
            owner_(owner) {}

        StageManager* owner_;
    };

    friend struct IteratorPair;

    StageManager();
    virtual ~StageManager();

    StagePtr new_stage(AvailablePartitioner partitioner=PARTITIONER_FRUSTUM);
    StagePtr stage(StageID s);
    StagePtr destroy_stage(StageID s);
    std::size_t stage_count() const;
    bool has_stage(StageID stage_id) const;

    void fixed_update(float dt) override;
    void update(float dt) override;
    void late_update(float dt) override;

    void destroy_all_stages();

    IteratorPair each_stage();

    /* Implementation for TypedDestroyableObject (INTERNAL) */
    void destroy_object(Stage* object);
    void destroy_object_immediately(Stage* object);

protected:
    void clean_up();

    typedef Polylist<StageNode, Stage> StagePool;
    typedef StageNodeManager<StagePool, StageID, Stage> StageList;

    StagePool* pool_ = nullptr;
    StageList* manager_ = nullptr;
};

}
