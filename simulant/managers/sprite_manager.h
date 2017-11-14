#pragma once

#include "../generic/manager.h"
#include "../nodes/sprite.h"
#include "./window_holder.h"

namespace smlt {

typedef generic::TemplatedManager<Sprite, SpriteID> TemplatedSpriteManager;

typedef sig::signal<void (SpriteID)> SpriteCreatedSignal;
typedef sig::signal<void (SpriteID)> SpriteDestroyedSignal;


class SpriteManager :
    public TemplatedSpriteManager,
    public virtual WindowHolder {

    DEFINE_SIGNAL(SpriteCreatedSignal, signal_sprite_created);
    DEFINE_SIGNAL(SpriteDestroyedSignal, signal_sprite_destroyed);

public:
    SpriteManager(Window* window, Stage* stage);

    SpritePtr new_sprite();
    SpritePtr new_sprite_from_file(
        const unicode& filename,
        uint32_t frame_width, uint32_t frame_height,
        const SpritesheetAttrs &attrs=SpritesheetAttrs()
    );

    SpritePtr new_sprite_from_texture(TextureID texture_id,
        uint32_t frame_width, uint32_t frame_height,
        const SpritesheetAttrs &attrs=SpritesheetAttrs()
    );

    SpritePtr sprite(SpriteID s);
    bool has_sprite(SpriteID s) const;
    SpritePtr delete_sprite(SpriteID s);
    std::size_t sprite_count() const;
    void delete_all();

    Property<SpriteManager, Stage> stage = { this, &SpriteManager::stage_ };
private:
    Stage* stage_ = nullptr;
};

}
