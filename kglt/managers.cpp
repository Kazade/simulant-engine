
#include "managers.h"
#include "background.h"
#include "window_base.h"
#include "scene.h"
#include "stage.h"

#include "utils/ownable.h"

namespace kglt {

//============== START BACKGROUNDS ==========
BackgroundManager::BackgroundManager(WindowBase* window):
    window_(window) {

}

void BackgroundManager::update(double dt) {
    //Update the backgrounds
    for(auto background_pair: BackgroundManager::__objects()) {
        auto* bg = background_pair.second.get();
        bg->update(dt);
    }
}

BackgroundID BackgroundManager::new_background() {
    BackgroundID bid = BackgroundManager::manager_new();
    return bid;
}

BackgroundID BackgroundManager::new_background_from_file(const unicode& filename, float scroll_x, float scroll_y) {
    BackgroundID result = new_background();
    try {
        background(result)->set_texture(window_->scene().new_texture_from_file(filename));
        background(result)->set_horizontal_scroll_rate(scroll_x);
        background(result)->set_vertical_scroll_rate(scroll_y);
    } catch(...) {
        delete_background(result);
        throw;
    }

    return result;
}

BackgroundPtr BackgroundManager::background(BackgroundID bid) {
    return BackgroundManager::manager_get(bid);
}

bool BackgroundManager::has_background(BackgroundID bid) const {
    return BackgroundManager::manager_contains(bid);
}

void BackgroundManager::delete_background(BackgroundID bid) {
    BackgroundManager::manager_delete(bid);
}

uint32_t BackgroundManager::background_count() const {
    return BackgroundManager::manager_count();
}

//============== END BACKGROUNDS ============


//=========== START STAGES ==================

StageManager::StageManager(WindowBase *window):
    window_(window) {

    default_stage_id_ = new_stage(); //Create the default stage
}

StageID StageManager::new_stage(AvailablePartitioner partitioner) {
    return StageManager::manager_new(StageID(), partitioner);
}

uint32_t StageManager::stage_count() const {
    return StageManager::manager_count();
}

/**
 * @brief StageManager::stage
 * @return A shared_ptr to the default stage
 *
 * We don't return a ProtectedPtr because it makes usage a nightmare. Stages don't suffer the same potential
 * threading issues as other objects as they are the highest level object. Returning a weak_ptr means that
 * we retain ownership, and calling code won't die if the stage goes missing.
 */
StagePtr StageManager::stage() {
    return StageManager::manager_get(default_stage_id_);
}

StagePtr StageManager::stage(StageID s) {
    if(!s) {
        return stage();
    }

    return StageManager::manager_get(s);
}

void StageManager::delete_stage(StageID s) {
    //Recurse through the tree, destroying all children
    stage(s)->apply_recursively_leaf_first(&ownable_tree_node_destroy, false);

    StageManager::manager_delete(s);
}

void StageManager::update(double dt) {
    //Update the stages
    for(auto stage_pair: StageManager::__objects()) {
        GenericTreeNode* root = stage_pair.second.get();
        root->apply_recursively([=](GenericTreeNode* node) -> void {
            node->as<Updateable>()->update(dt);
        });
    }
}


void StageManager::print_tree() {
    for(auto stage: StageManager::__objects()) {
        uint32_t counter = 0;
        print_tree(stage.second.get(), counter);
    }
}

void StageManager::print_tree(GenericTreeNode* node, uint32_t& level) {
    for(uint32_t i = 0; i < level; ++i) {
        std::cout << "    ";
    }

    std::cout << *dynamic_cast<Printable*>(node) << std::endl;

    level += 1;
    for(auto child: node->children()) {
        print_tree(child, level);
    }
    level -= 1;
}

// ============= END STAGES ===========

}
