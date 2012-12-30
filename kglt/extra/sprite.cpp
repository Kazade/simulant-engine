#include "kglt/procedural/mesh.h"
#include "kglt/shortcuts.h"
#include "sprite.h"

namespace kglt {
namespace extra {

Sprite::Sprite(Scene& scene, SubSceneID subscene):
    scene_(scene),
    subscene_(subscene) {

    mesh_id_ = scene_.subscene(subscene_).new_mesh();
    entity_id_ = scene_.subscene(subscene_).new_entity();
    kglt::procedural::mesh::rectangle(scene_.subscene(subscene_).mesh(mesh_id_), 1.0, 1.0);
    scene_.subscene(subscene_).entity(entity_id()).set_mesh(mesh_id_); //Rebuild the entity
}

void Sprite::add_animation(const std::string& anim_name, const std::vector<TextureID>& frames, double duration) {
    SubScene& subscene = scene().subscene(subscene_id());

    Material& mat = subscene.material(subscene.new_material(scene().default_material()));
    mat.technique().pass(0).set_animated_texture_unit(0, frames, duration);
    mat.technique().pass(0).set_blending(BLEND_ALPHA);
    animations_[anim_name] = mat.id();

    if(animations_.size() == 1 && current_animation_.empty()) {
        set_active_animation(anim_name);
    }
}

void Sprite::set_active_animation(const std::string &anim_name) {
    SubScene& subscene = scene().subscene(subscene_id());

    current_animation_ = anim_name;
    kglt::Mesh& mesh = subscene.mesh(mesh_id_);

    mesh.submesh(mesh.submesh_ids()[0]).set_material(animations_[anim_name]);
}

void Sprite::set_render_dimensions(float width, float height) {
    SubScene& subscene = scene().subscene(subscene_id());

    Mesh& mesh = subscene.mesh(mesh_id_);
    kglt::procedural::mesh::rectangle(mesh, width, height);

    subscene.entity(entity_id()).set_mesh(mesh_id_); //Rebuild the entity
    set_active_animation(current_animation_); //Re-set the current animation
}

void Sprite::move_to(float x, float y, float z) {
    SubScene& subscene = scene().subscene(subscene_id());
    subscene.entity(entity_id()).move_to(x, y, z);
}

}
}
