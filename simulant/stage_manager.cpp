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


#include "stage_manager.h"
#include "window.h"
#include "stage.h"
#include "nodes/camera.h"
#include "render_sequence.h"
#include "loader.h"

#include "renderers/batching/render_queue.h"
#include "generic/manual_manager.h"

namespace smlt {


//=========== START STAGES ==================

StageManager::StageManager(Window* window):
    window_(window),
    stage_manager_(new ManualManager<Stage, StageID>()) {

}

StagePtr StageManager::new_stage(AvailablePartitioner partitioner) {
    auto ret = stage_manager_->make(this->window_, partitioner);
    signal_stage_added_(ret->id());
    return ret;
}

std::size_t StageManager::stage_count() const {
    return stage_manager_->size();
}

/**
 * @brief StageManager::stage
 * @return A shared_ptr to the stage
 *
 * We don't return a ProtectedPtr because it makes usage a nightmare. Stages don't suffer the same potential
 * threading issues as other objects as they are the highest level object. Returning a weak_ptr means that
 * we retain ownership, and calling code won't die if the stage goes missing.
 */

StagePtr StageManager::stage(StageID s) {
    return stage_manager_->get(s);
}

StagePtr StageManager::destroy_stage(StageID s) {
    stage_manager_->destroy(s);
    signal_stage_removed_(s);
    return nullptr;
}


static void _fixed_update(Stage* stage, float* dt) {
    if(!stage->is_part_of_active_pipeline()) {
        return;
    }

    stage->fixed_update(*dt);

    for(auto& stage_node: stage->each_descendent()) {
        stage_node.fixed_update(*dt);
    }
}

void StageManager::fixed_update(float dt) {
    /* safe_each locks the entire loop */
    stage_manager_->safe_each(_fixed_update, &dt);
}

static void _late_update(Stage* stage, float* dt) {
    if(!stage->is_part_of_active_pipeline()) {
        return;
    }

    stage->late_update(*dt);
    for(auto& stage_node: stage->each_descendent()) {
        stage_node.late_update(*dt);
    }
}

void StageManager::late_update(float dt) {
    stage_manager_->safe_each(_late_update, &dt);
}

static void _update(Stage* stage, float* dt) {
    if(!stage->is_part_of_active_pipeline()) {
        return;
    }

    stage->update(*dt);
    for(auto& stage_node: stage->each_descendent()) {
        stage_node.update(*dt);
    }
}

void StageManager::update(float dt) {
    //Update the stages
    stage_manager_->safe_each(_update, &dt);
}

void StageManager::print_tree() {
    for(auto stage: stage_manager_->_each()) {
        uint32_t counter = 0;
        print_tree(stage, counter);
    }
}

void StageManager::print_tree(StageNode *node, uint32_t& level) {
    for(uint32_t i = 0; i < level; ++i) {
        std::cout << "    ";
    }

    std::cout << *dynamic_cast<Printable*>(node) << std::endl;

    level += 1;
    for(auto& child: node->each_child()) {
        print_tree(&child, level);
    }
    level -= 1;
}

void StageManager::clean_up() {
    stage_manager_->clean_up();
}

bool StageManager::has_stage(StageID stage_id) const {
    return stage_manager_->contains(stage_id);
}

void StageManager::destroy_all_stages() {
    stage_manager_->destroy_all();
}

StageManager::stage_iterator_pair StageManager::each_stage() {
    return stage_manager_->_each();
}

void StageManager::destroy_object(Stage* object) {
    stage_manager_->destroy(object->id());
}

void StageManager::destroy_object_immediately(Stage* object) {
    stage_manager_->destroy_immediately(object->id());
}

// ============= END STAGES ===========

}
