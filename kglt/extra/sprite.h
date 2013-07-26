#ifndef ADDITIONAL_SPRITE_H
#define ADDITIONAL_SPRITE_H

#include <unordered_map>
#include "../generic/managed.h"
#include "../types.h"
#include "../base.h"

namespace kglt {

class Scene;

namespace extra {

/**
  USAGE:

  TextureID tex = stage.new_texture_from_file("sprite.png");

  //Split a texture into parts, 64 pixels wide
  Sprite::ptr sprite = Sprite::create(stage, tex, FrameSize(64, 64));
  sprite->add_animation("running", FrameRange(0, 15), 5.0);
  sprite->update(window.deltatime());
*/

struct FrameSize {
    FrameSize(uint32_t width, uint32_t height):
        width(width),
        height(height) {}

    uint32_t width;
    uint32_t height;
};

struct FrameRange {
    FrameRange(uint32_t start, uint32_t end):
        start(start),
        end(end) {}

    uint32_t start;
    uint32_t end;
};

class Sprite :
    public Managed<Sprite>,
    public MoveableActorHolder {

public:
    typedef std::shared_ptr<Sprite> ptr;

    Sprite(Scene& scene, StageID stage_id, const std::string &image_path, const FrameSize& frame_size);
    ~Sprite();

    void add_animation(
        const std::string& anim_name,
        const FrameRange& frames,
        double duration
    );

    void set_next_animation(const std::string& anim_name);
    void set_active_animation(const std::string& anim_name);
    void move_to(float x, float y, float z);
    void rotate_to(float angle, float x, float y, float z);

    void set_render_dimensions(float width, float height);
    void set_visible(bool value=true);

    MaterialID material();

    void update(double dt);

    ActorID actor_id() const { return actor_id_; }
    StageID stage_id() const { return stage_id_; }

    bool init();
private:
    struct Animation {
        Animation(double duration, const FrameRange& frames):
            duration(duration),
            frames(frames) {}

        double duration;
        FrameRange frames;
    };

    std::unordered_map<std::string, Animation> animations_;
    std::string image_path_;
    StageID stage_id_;
    ActorID actor_id_;

    MaterialID material_id_;
    FrameSize frame_size_;

    uint32_t current_frame_ = 0;
    uint32_t next_frame_ = 0;
    double interp_ = 0.0;
    uint32_t image_width_ = 0;
    uint32_t image_height_ = 0;

    std::string current_animation_;
    std::string next_animation_;

    void update_texture_coordinates();
};

}
}

#endif // SPRITE_H
