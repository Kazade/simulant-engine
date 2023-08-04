#pragma once

#include "../sprite.h"
#include "../../managers/window_holder.h"
#include "../stage_node_manager.h"
#include "../stage_node_pool.h"
#include "../../path.h"

namespace smlt {

typedef StageNodeManager<StageNodePool, StageNodeID, Sprite> TemplatedSpriteManager;

typedef sig::signal<void (StageNodeID)> SpriteCreatedSignal;
typedef sig::signal<void (StageNodeID)> SpriteDestroyedSignal;

class SpriteManager {

    DEFINE_SIGNAL(SpriteCreatedSignal, signal_sprite_created);
    DEFINE_SIGNAL(SpriteDestroyedSignal, signal_sprite_destroyed);

    friend class Sprite;

public:
    SpriteManager(Stage* stage, StageNodePool *pool);
    virtual ~SpriteManager();

    SpritePtr new_sprite();
    SpritePtr new_sprite_from_file(const smlt::Path& filename,
        uint32_t frame_width, uint32_t frame_height,
        const SpritesheetAttrs &attrs=SpritesheetAttrs()
    );

    SpritePtr new_sprite_from_texture(TexturePtr texture,
        uint32_t frame_width, uint32_t frame_height,
        const SpritesheetAttrs &attrs=SpritesheetAttrs()
    );

    SpritePtr sprite(StageNodeID s);
    bool has_sprite(StageNodeID s) const;
    SpritePtr destroy_sprite(StageNodeID s);
    std::size_t sprite_count() const;
    void destroy_all();

    void destroy_object(Sprite* skybox);
    void destroy_object_immediately(Sprite* skybox);
private:
    Stage* stage_ = nullptr;
    sig::connection clean_up_conn_;

    std::shared_ptr<TemplatedSpriteManager> sprite_manager_;

public:
    Property<decltype(&SpriteManager::stage_)> stage = { this, &SpriteManager::stage_ };
};

}
