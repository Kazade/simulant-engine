//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//


#define DEFINE_STAGENODEPOOL
#include "nodes/stage_node_pool.h"

#include "stage_manager.h"
#include "application.h"
#include "stage.h"
#include "nodes/camera.h"
#include "compositor.h"
#include "loader.h"

#include "renderers/batching/render_queue.h"

namespace smlt {


//=========== START STAGES ==================

StageManager::StageManager():
    pool_(new StagePool(4)),
    manager_(new StageList(pool_)) {

}

StageManager::~StageManager() {
    delete manager_;
    delete pool_;
}

StagePtr StageManager::new_stage(AvailablePartitioner partitioner) {
    auto pool = get_app()->stage_node_pool.get();

    auto ret = manager_->make(this, pool, partitioner);
    signal_stage_added_(ret->id());
    return ret;
}

std::size_t StageManager::stage_count() const {
    return pool_->size();
}

/**
 * @brief StageManager::stage
 * @return A shared_ptr to the stage
 */

StagePtr StageManager::stage(StageID s) {
    return manager_->get(s.value());
}

StagePtr StageManager::destroy_stage(StageID s) {
    manager_->destroy(s.value());
    signal_stage_removed_(s);
    return nullptr;
}

void StageManager::fixed_update(float dt) {
    /* safe_each locks the entire loop */
    for(auto stage: *manager_) {
        if(!stage->is_part_of_active_pipeline()) {
            continue;
        }

        stage->fixed_update(dt);

        for(auto& node: stage->each_descendent()) {
            node.fixed_update(dt);
        }
    }
}

void StageManager::late_update(float dt) {
    /* We only update stages that are part of an active
     * pipeline, but we *always* clean up dead objects */
    for(auto stage: *manager_) {
        if(stage->is_part_of_active_pipeline()) {
            stage->late_update(dt);

            for(auto& node: stage->each_descendent()) {
                node.late_update(dt);
            }
        }

        stage->clean_up_dead_objects();
    }
}

void StageManager::update(float dt) {
    //Update the stages
    for(auto stage: *manager_) {
        if(!stage->is_part_of_active_pipeline()) {
            continue;
        }

        stage->update(dt);

        for(auto& node: stage->each_descendent()) {
            node.update(dt);
        }
    }
}

void StageManager::clean_destroyed_stages() {
    manager_->clean_up();
}

bool StageManager::has_stage(StageID stage_id) const {
    return manager_->contains(stage_id);
}

void StageManager::destroy_all_stages() {
    manager_->destroy_all();
}

StageManager::IteratorPair StageManager::each_stage() {
    return IteratorPair(this);
}

void StageManager::destroy_object(Stage* object) {
    manager_->destroy(object->id());
}

void StageManager::destroy_object_immediately(Stage* object) {
    manager_->destroy_immediately(object->id());
}

std::list<Stage*>::iterator StageManager::IteratorPair::begin() {
    return owner_->manager_->begin();
}

std::list<Stage*>::iterator StageManager::IteratorPair::end() {
    return owner_->manager_->end();
}

// ============= END STAGES ===========

}
