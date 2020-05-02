#pragma once

#include "../nodes/sprite.h"
#include "./window_holder.h"

namespace smlt {

template<typename T, typename IDType, typename ...Subtypes>
class ManualManager;

typedef ManualManager<Sprite, SpriteID> TemplatedSpriteManager;

typedef sig::signal<void (SpriteID)> SpriteCreatedSignal;
typedef sig::signal<void (SpriteID)> SpriteDestroyedSignal;


class SpriteManager :
    public virtual WindowHolder {

    DEFINE_SIGNAL(SpriteCreatedSignal, signal_sprite_created);
    DEFINE_SIGNAL(SpriteDestroyedSignal, signal_sprite_destroyed);

    friend class Sprite;

public:
    SpriteManager(Window* window, Stage* stage);
    virtual ~SpriteManager();

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
    SpritePtr destroy_sprite(SpriteID s);
    std::size_t sprite_count() const;
    void destroy_all();

private:
    Stage* stage_ = nullptr;
    sig::connection clean_up_conn_;

    std::shared_ptr<TemplatedSpriteManager> sprite_manager_;

public:
    Property<decltype(&SpriteManager::stage_)> stage = { this, &SpriteManager::stage_ };
};

}
