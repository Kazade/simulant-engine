#ifndef ADDITIONAL_SPRITE_H
#define ADDITIONAL_SPRITE_H

#include <map>
#include "../generic/managed.h"
#include "../types.h"

namespace kglt {

class Scene;

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
    public Managed<Sprite> {

public:
    typedef std::shared_ptr<Sprite> ptr;

    Sprite(Scene& scene, SubSceneID subscene=DefaultSubSceneID);

    void add_animation(const std::string& anim_name, const std::vector<TextureID>& frames, double duration);

    void set_next_animation(const std::string& anim_name);
    void set_active_animation(const std::string& anim_name);
    void move_to(float x, float y, float z);
    void set_render_dimensions(float width, float height);
    void set_visible(bool value=true);

    MaterialID material();
    Scene& scene() { return scene_; }
    SubSceneID subscene_id() const { return subscene_; }
    EntityID entity_id() { return entity_id_; }

private:
    Scene& scene_;
    SubSceneID subscene_;

    std::string current_animation_;
    std::string next_animation_;

    std::map<std::string, MaterialID> animations_;

    MeshID mesh_id_;
    EntityID entity_id_;
};

}
}

#endif // SPRITE_H
