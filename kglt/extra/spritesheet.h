#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "kazbase/unicode.h"

namespace kglt {
namespace extra {

class SpriteSheet;

struct Animation {
    Animation(uint32_t start, uint32_t end, float fps);
};

typedef std::map<unicode, Animation> AnimationDef;

/* USAGE:
 *
 * auto spritesheet = SpriteSheet::create("some_file.png", FrameSize(64, 64));
 * AnimationDef animations = {
 *    "stand", Animation(0, 5, 5.0),
 *    "walk", Animation(6, 9, 5.0)
 * };
 *
 * auto sprite = spritesheet.new_animated_sprite(animations);
 *
 * sprite.set_animation("stand");
 * sprite.set_next_animation("walk");

/**
 * @brief A class for managing multiple sprites that provides functions for
 * playing different animations.
 */
class AnimatedSprite {
public:
    void set_animation(const unicode& animation, bool loop=true);
    void set_next_animation(const unicode& animation, bool loop=true);

private:
    friend class SpriteSheet;
    AnimatedSprite(const SpriteSheet& sheet, const std::map<unicode, Sprite::ptr>& sprites);

    std::map<unicode, Sprite::ptr> sprites_;
};

class SpriteSheet {
public:
    SpriteSheet(const unicode& filename, const FrameSize& frame_size);

    Sprite::ptr new_sprite(uint32_t start_frame, uint32_t end_frame);
    Sprite::ptr new_sprite(uint32_t frame);

    AnimatedSprite::ptr new_animated_sprite(const AnimationDef& animations);
};

}
}

#endif // SPRITESHEET_H
