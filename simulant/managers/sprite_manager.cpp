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

SpritePtr SpriteManager::new_sprite() {
    auto s = TemplatedSpriteManager::make(this, window->_sound_driver()).fetch();
    s->set_parent(stage_->id());
    signal_sprite_created_(s->id());
    return s;
}

SpritePtr SpriteManager::new_sprite_from_file(const unicode &filename, uint32_t frame_Width, uint32_t frame_height, const SpritesheetAttrs& attrs) {
    TextureID t = stage_->assets->new_texture_from_file(
        filename,
        TextureFlags(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_POINT)
    );

    return new_sprite_from_texture(t, frame_Width, frame_height, attrs);
}

SpritePtr SpriteManager::new_sprite_from_texture(TextureID texture_id, uint32_t frame_width, uint32_t frame_height, const SpritesheetAttrs& attrs) {
    SpritePtr s = new_sprite();

    try {
        s->set_spritesheet(texture_id, frame_width, frame_height, attrs);

        // Set the render dimensions to match the image size by default
        s->set_render_dimensions(frame_width, frame_height);
    } catch(...) {
        delete_sprite(s->id());
        throw;
    }

    return s;
}

SpritePtr SpriteManager::sprite(SpriteID s) {
    return TemplatedSpriteManager::get(s);
}

bool SpriteManager::has_sprite(SpriteID s) const {
    return TemplatedSpriteManager::contains(s);
}

SpritePtr SpriteManager::delete_sprite(SpriteID s) {
    TemplatedSpriteManager::destroy(s);
    return nullptr;
}

std::size_t SpriteManager::sprite_count() const {
    return TemplatedSpriteManager::count();
}

}
