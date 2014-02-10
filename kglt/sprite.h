#ifndef SPRITE_H
#define SPRITE_H

#include "generic/managed.h"

namespace kglt {

class Sprite : public Managed<Sprite> {
public:
    bool init() override;
    void cleanup() override;

    Sprite(Stage* stage, SpriteID id);

    void set_texture(
        TextureID texture_id,
        uint32_t frame_width,
        uint32_t frame_height,
        uint32_t margin=0, uint32_t spacing=0
    );

private:
    ActorID actor_id_;
};

}
#endif // SPRITE_H
