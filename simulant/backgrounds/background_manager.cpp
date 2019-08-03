
#include "background_manager.h"
#include "../window.h"
#include "../asset_manager.h"
#include "../generic/manual_manager.h"

namespace smlt {

//============== START BACKGROUNDS ==========
BackgroundManager::BackgroundManager(Window* window):
    window_(window),
    backgrounds_(new Manager()) {

}

BackgroundManager::~BackgroundManager() {
    destroy_all_backgrounds();
}

void BackgroundManager::update(float dt) {
    //Update the backgrounds
    backgrounds_->each([dt](uint32_t, BackgroundPtr bg) {
       bg->update(dt);
    });
}

void BackgroundManager::clean_up() {
    backgrounds_->clean_up();
}

BackgroundPtr BackgroundManager::new_background(BackgroundType type) {
    return backgrounds_->make(this, type);
}

BackgroundPtr BackgroundManager::new_background_as_scrollable_from_file(const unicode& filename, float scroll_x, float scroll_y) {
    BackgroundPtr bg = new_background(BACKGROUND_TYPE_SCROLL);
    try {
        bg->set_texture(window_->shared_assets->new_texture_from_file(filename));
        bg->set_horizontal_scroll_rate(scroll_x);
        bg->set_vertical_scroll_rate(scroll_y);
    } catch(...) {
        destroy_background(bg->id());
        throw;
    }

    return bg;
}

BackgroundPtr BackgroundManager::new_background_as_animated_from_file(const unicode& filename) {
    BackgroundPtr bg = new_background(BACKGROUND_TYPE_ANIMATED);
    try {
        bg->set_texture(window_->shared_assets->new_texture_from_file(filename));
    } catch(...) {
        destroy_background(bg->id());
        throw;
    }

    return bg;
}

BackgroundPtr BackgroundManager::background(BackgroundID bid) {
    return backgrounds_->get(bid);
}

bool BackgroundManager::has_background(BackgroundID bid) const {
    return backgrounds_->contains(bid);
}

BackgroundPtr BackgroundManager::destroy_background(BackgroundID bid) {
    backgrounds_->destroy(bid);
    return nullptr;
}

uint32_t BackgroundManager::background_count() const {
    return backgrounds_->size();
}

void BackgroundManager::destroy_all_backgrounds() {
    backgrounds_->destroy_all();
}

//============== END BACKGROUNDS ============

}
