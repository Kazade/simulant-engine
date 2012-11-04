#ifndef ADDITIONAL_SPRITE_H
#define ADDITIONAL_SPRITE_H

#include "../generic/creator.h"
#include "../scene.h"

namespace kglt {
namespace extra {

/**
  USAGE:

  //Split a texture into parts, 64 pixels wide
  kglt::helper::SpriteStripLoader loader(scene, "animation.png", 64);
  MaterialID running_anim = kglt::create_material_from_textures(scene, loader.load_frames());

  Sprite::ptr sprite = Sprite::create();
  sprite->add_animation("running", running_anim);
  sprite->update(window.deltatime());
*/

class Sprite :
    public generic::Creator<Sprite> {

public:
    typedef std::tr1::shared_ptr<Sprite> ptr;

    Sprite(Scene& scene);

    void add_animation(const std::string& anim_name, const std::vector<TextureID>& frames, double duration);

    void set_next_animation(const std::string& anim_name);
    void set_active_animation(const std::string& anim_name);
    void move_to(float x, float y, float z);
    void set_render_dimensions(float width, float height);

    MaterialID material();
    Scene& scene() { return scene_; }

    EntityID entity() { return entity_id_; }
private:
    Scene& scene_;

    std::string current_animation_;
    std::string next_animation_;

    std::map<std::string, MaterialID> animations_;

    MeshID mesh_id_;
    EntityID entity_id_;
};

}
}

#endif // SPRITE_H
