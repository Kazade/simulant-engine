#include "sprite_manager.h"
#include "../texture.h"
#include "../window.h"
#include "../stage.h"

namespace smlt {

SpriteManager::SpriteManager(Window* window, Stage* stage):
    WindowHolder(window),
    stage_(stage)   {

}

void SpriteManager::delete_all() {
    objects_.clear();
}

SpriteID SpriteManager::new_sprite() {
    SpriteID s = TemplatedSpriteManager::make(this, window->_sound_driver());
    sprite(s)->set_parent(stage_->id());
    signal_sprite_created_(s);
    return s;
}

SpriteID SpriteManager::new_sprite_from_file(const unicode& filename, uint32_t frame_width, uint32_t frame_height, uint32_t margin, uint32_t spacing, std::pair<uint32_t, uint32_t> padding) {
    SpriteID s = new_sprite();
    TextureID t = stage_->assets->new_texture_from_file(
        filename,
        TextureFlags(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_NEAREST)
    );
    try {
        sprite(s)->set_spritesheet(t, frame_width, frame_height, margin, spacing, padding);

        // Set the render dimensions to match the image size by default
        sprite(s)->set_render_dimensions(frame_width, frame_height);
    } catch(...) {
        delete_sprite(s);
        throw;
    }

    return s;
}

SpritePtr SpriteManager::sprite(SpriteID s) {
    return TemplatedSpriteManager::get(s).lock().get();
}

bool SpriteManager::has_sprite(SpriteID s) const {
    return TemplatedSpriteManager::contains(s);
}

void SpriteManager::delete_sprite(SpriteID s) {
    TemplatedSpriteManager::destroy(s);
}

uint32_t SpriteManager::sprite_count() const {
    return TemplatedSpriteManager::count();
}

}
