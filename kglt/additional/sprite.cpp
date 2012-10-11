#include "kglt/procedural/mesh.h"
#include "kglt/shortcuts.h"
#include "sprite.h"

namespace kglt {
namespace additional {

Sprite::Sprite(Scene& scene):
    scene_(scene) {

    mesh_id_ = scene_.new_mesh();    
    kglt::procedural::mesh::rectangle(scene_.mesh(mesh_id_), 1.0, 1.0);
}

void Sprite::add_animation(const std::string& anim_name, const std::vector<TextureID>& frames, double duration) {
    Material& mat = kglt::return_new_material(scene_);
    mat.technique().new_pass(scene_.default_shader());
    mat.technique().pass(0).set_animated_texture_unit(0, frames, duration);
    mat.technique().pass(0).set_blending(BLEND_ALPHA);
    animations_[anim_name] = mat.id();

    if(animations_.size() == 1 && current_animation_.empty()) {
        set_active_animation(anim_name);
    }
}

void Sprite::set_active_animation(const std::string &anim_name) {
    current_animation_ = anim_name;
    scene_.mesh(mesh_id_).apply_material(animations_[anim_name]);
}

void Sprite::set_render_dimensions(float width, float height) {
    Mesh& mesh = scene().mesh(mesh_id_);
    kglt::procedural::mesh::rectangle(mesh, width, height);
}

void Sprite::move_to(float x, float y, float z) {
    scene_.mesh(mesh_id_).move_to(x, y, z);
}

}
}
