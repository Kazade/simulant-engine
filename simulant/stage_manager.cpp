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

namespace smlt {


//=========== START STAGES ==================

StageManager::StageManager(Window* window):
    window_(window) {

}

StagePtr StageManager::new_stage(AvailablePartitioner partitioner) {
    auto ret = stage_manager_.make(this->window_, partitioner);
    signal_stage_added_(ret->id());
    return ret;
}

std::size_t StageManager::stage_count() const {
    return stage_manager_.size();
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
    return stage_manager_.get(s);
}

StagePtr StageManager::delete_stage(StageID s) {
    stage_manager_.destroy(s);
    signal_stage_removed_(s);
    return nullptr;
}

void StageManager::fixed_update(float dt) {
    stage_manager_.each([dt](uint32_t, StagePtr stage) {
        TreeNode* root = stage;

        root->each_descendent_and_self([=](uint32_t, TreeNode* node) {
            StageNode* stage_node = static_cast<StageNode*>(node);
            stage_node->fixed_update(dt);
        });
    });
}

void StageManager::late_update(float dt) {
    stage_manager_.each([dt](uint32_t, StagePtr stage) {
        TreeNode* root = stage;

        root->each_descendent_and_self([=](uint32_t, TreeNode* node) {
            StageNode* stage_node = static_cast<StageNode*>(node);
            stage_node->late_update(dt);
        });
    });
}


void StageManager::update(float dt) {
    //Update the stages
    stage_manager_.each([dt](uint32_t, StagePtr stage) {
        TreeNode* root = stage;

        root->each_descendent_and_self([=](uint32_t, TreeNode* node) {
            StageNode* stage_node = static_cast<StageNode*>(node);
            stage_node->update(dt);
        });
    });
}


void StageManager::print_tree() {
    stage_manager_.each([this](uint32_t, StagePtr stage) {
        uint32_t counter = 0;
        print_tree(stage, counter);
    });
}

void StageManager::print_tree(StageNode *node, uint32_t& level) {
    for(uint32_t i = 0; i < level; ++i) {
        std::cout << "    ";
    }

    std::cout << *dynamic_cast<Printable*>(node) << std::endl;

    level += 1;
    node->each_child([&](uint32_t, TreeNode* child) {
        print_tree(static_cast<StageNode*>(child), level);
    });
    level -= 1;
}

bool StageManager::has_stage(StageID stage_id) const {
    return stage_manager_.contains(stage_id);
}

void StageManager::delete_all_stages() {
    stage_manager_.clear();
}

void StageManager::each_stage(std::function<void (uint32_t, Stage*)> func) {
    stage_manager_.each(func);
}

// ============= END STAGES ===========

}
