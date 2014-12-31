
#include "managers.h"
#include "background.h"
#include "window_base.h"
#include "stage.h"
#include "ui_stage.h"
#include "camera.h"
#include "utils/ownable.h"
#include "render_sequence.h"

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
        background(result)->set_texture(window_->new_texture_from_file(filename));
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

//=============== START CAMERAS ============

CameraManager::CameraManager(WindowBase *window):
    window_(window) {

}

CameraID CameraManager::new_camera() {
    CameraID new_camera = CameraManager::manager_new();

    return new_camera;
}

CameraID CameraManager::new_camera_with_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    /*
     *  Instantiates a camera with an orthographic projection. If both left and right are zero then they default to 0 and window.width()
     *  respectively. If bottom and top are zero, then they default to window.height() and 0 respectively. So top left is 0,0
     */
    CameraID new_camera_id = new_camera();

    if(!left && !right) {
        right = window_->width();
    }

    if(!bottom && !top) {
        bottom = window_->height();
    }

    camera(new_camera_id)->set_orthographic_projection(left, right, bottom, top, near, far);

    return new_camera_id;
}

CameraID CameraManager::new_camera_for_ui() {
    return new_camera_with_orthographic_projection(0, window_->width(), window_->height(), 0, -1, 1);
}

CameraPtr CameraManager::camera(CameraID c) {
    return CameraManager::manager_get(c);
}

void CameraManager::delete_camera(CameraID cid) {
    try {
        //Remove any associated proxy
        auto cam = camera(cid);
        if(cam->has_proxy()) {
            cam->proxy().stage()->evict_camera(cid);
        }
    } catch(DoesNotExist<Camera>&) {
        // If the camera has already been deleted, do nothing
        return;
    }

    CameraManager::manager_delete(cid);
}

uint32_t CameraManager::camera_count() const {
    return CameraManager::manager_count();
}

const bool CameraManager::has_camera(CameraID id) const {
    return CameraManager::manager_contains(id);
}

//============== END CAMERAS ================


//=========== START STAGES ==================

StageManager::StageManager(WindowBase *window):
    window_(window) {

}

StageID StageManager::new_stage(AvailablePartitioner partitioner) {
    return StageManager::manager_new(StageID(), partitioner);
}

uint32_t StageManager::stage_count() const {
    return StageManager::manager_count();
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
    return StagePtr(*this, s);
}

void StageManager::delete_stage(StageID s) {
    try {
        //Recurse through the tree, destroying all children
        stage(s)->apply_recursively_leaf_first(&ownable_tree_node_destroy, false);
    } catch(DoesNotExist<Stage>&) {
        return;
    }

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

// ============= UI STAGES ============

UIStageManager::UIStageManager(WindowBase *window):
    window_(window) {

}

UIStageID UIStageManager::new_ui_stage() {
    return UIStageManager::manager_new();
}

UIStagePtr UIStageManager::ui_stage(UIStageID s) {
    return UIStageManager::manager_get(s);
}

void UIStageManager::delete_ui_stage(UIStageID s) {
    UIStageManager::manager_delete(s);
}

uint32_t UIStageManager::ui_stage_count() const {
    return UIStageManager::manager_count();
}

// =========== END UI STAGES ==========
}
