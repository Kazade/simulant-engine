
#include "managers.h"
#include "background.h"
#include "window_base.h"
#include "scene.h"

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


}
