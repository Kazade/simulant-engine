#include "sprite_manager.h"
#include "../../texture.h"
#include "../../window.h"
#include "../../stage.h"
#include "../sprite.h"
#include "../stage_node_manager.h"
#include "../../application.h"

namespace smlt {

SpriteManager::SpriteManager(Stage* stage, StageNodePool* pool):
    stage_(stage),
    sprite_manager_(new TemplatedSpriteManager(pool)) {

    clean_up_conn_ = get_app()->signal_late_update().connect([&](float) {
       sprite_manager_->clean_up();
    });
}

SpriteManager::~SpriteManager() {
    clean_up_conn_.disconnect();
}

void SpriteManager::destroy_all() {
    sprite_manager_->clear();
}

SpritePtr SpriteManager::new_sprite() {
    auto s = sprite_manager_->make(this, get_app()->sound_driver);
    s->set_parent(stage_);
    signal_sprite_created_(s->id());
    return s;
}

SpritePtr SpriteManager::new_sprite_from_file(const Path &filename, uint32_t frame_Width, uint32_t frame_height, const SpritesheetAttrs& attrs) {
    auto t = stage_->assets->new_texture_from_file(
        filename,
        TextureFlags(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_POINT)
    );

    return new_sprite_from_texture(t, frame_Width, frame_height, attrs);
}

SpritePtr SpriteManager::new_sprite_from_texture(TexturePtr texture, uint32_t frame_width, uint32_t frame_height, const SpritesheetAttrs& attrs) {
    SpritePtr s = new_sprite();

    try {
        s->set_spritesheet(texture, frame_width, frame_height, attrs);

        // Set the render dimensions to match the image size by default
        s->set_render_dimensions(frame_width, frame_height);
    } catch(...) {
        destroy_sprite(s->id());
        throw;
    }

    return s;
}

SpritePtr SpriteManager::sprite(StageNodeID s) {
    return sprite_manager_->get(s);
}

bool SpriteManager::has_sprite(StageNodeID s) const {
    return sprite_manager_->contains(s);
}

SpritePtr SpriteManager::destroy_sprite(StageNodeID s) {
    sprite_manager_->destroy(s);
    return nullptr;
}

std::size_t SpriteManager::sprite_count() const {
    return sprite_manager_->size();
}

void SpriteManager::destroy_object(Sprite* sprite) {
    auto id = sprite->id();
    sprite_manager_->destroy(id);
}

void SpriteManager::destroy_object_immediately(Sprite* sprite) {
    auto id = sprite->id();
    sprite_manager_->destroy_immediately(id);
}

}
