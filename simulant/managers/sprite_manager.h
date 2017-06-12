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
    SpriteManager(WindowBase* window, Stage* stage);

    SpriteID new_sprite();
    SpriteID new_sprite_from_file(
        const unicode& filename,
        uint32_t frame_Width, uint32_t frame_height,
        uint32_t margin=0, uint32_t spacing=0,
        std::pair<uint32_t, uint32_t> padding=std::make_pair(0, 0)
    );
    SpritePtr sprite(SpriteID s);
    bool has_sprite(SpriteID s) const;
    void delete_sprite(SpriteID s);
    uint32_t sprite_count() const;
    void delete_all();

    Property<SpriteManager, Stage> stage = { this, &SpriteManager::stage_ };
private:
    Stage* stage_ = nullptr;
};

}
