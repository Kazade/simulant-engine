
#include "background_manager.h"
#include "../window.h"
#include "../resource_manager.h"

namespace smlt {

//============== START BACKGROUNDS ==========
BackgroundManager::BackgroundManager(Window* window):
    window_(window) {

}

BackgroundManager::~BackgroundManager() {
    auto objects = BackgroundManager::__objects();
    for(auto background_pair: objects) {
        delete_background(background_pair.first);
    }
}

void BackgroundManager::update(float dt) {
    //Update the backgrounds
    for(auto background_pair: BackgroundManager::__objects()) {
        auto* bg = background_pair.second.get();
        bg->update(dt);
    }
}

BackgroundID BackgroundManager::new_background(BackgroundType type) {
    return make(this, type);
}

BackgroundID BackgroundManager::new_background_as_scrollable_from_file(const unicode& filename, float scroll_x, float scroll_y) {
    BackgroundPtr bg = new_background(BACKGROUND_TYPE_SCROLL).fetch();
    try {
        bg->set_texture(window_->shared_assets->new_texture_from_file(filename));
        bg->set_horizontal_scroll_rate(scroll_x);
        bg->set_vertical_scroll_rate(scroll_y);
    } catch(...) {
        delete_background(bg->id());
        throw;
    }

    return bg->id();
}

BackgroundID BackgroundManager::new_background_as_animated_from_file(const unicode& filename) {
    BackgroundPtr bg = new_background(BACKGROUND_TYPE_ANIMATED).fetch();
    try {
        bg->set_texture(window_->shared_assets->new_texture_from_file(filename));
    } catch(...) {
        delete_background(bg->id());
        throw;
    }

    return bg->id();
}

BackgroundPtr BackgroundManager::background(BackgroundID bid) {
    return BackgroundManager::get(bid).lock().get();
}

bool BackgroundManager::has_background(BackgroundID bid) const {
    return BackgroundManager::contains(bid);
}

void BackgroundManager::delete_background(BackgroundID bid) {
    BackgroundManager::destroy(bid);
}

uint32_t BackgroundManager::background_count() const {
    return BackgroundManager::count();
}

//============== END BACKGROUNDS ============

}
