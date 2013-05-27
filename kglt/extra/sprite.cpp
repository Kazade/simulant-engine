#include "sprite.h"

#include "../scene.h"
#include "../subscene.h"
#include "../procedural/mesh.h"
#include "../shortcuts.h"
#include "../entity.h"

namespace kglt {
namespace extra {

Sprite::Sprite(Scene& scene, SubSceneID subscene):
    scene_(scene),
    subscene_(scene.subscene(subscene)),
    entity_(nullptr) {

    entity_ = &subscene_.entity(subscene_.new_entity(subscene_.new_mesh()));

    kglt::procedural::mesh::rectangle(
        entity_->mesh().lock(), 1.0, 1.0
    );

    //FIXME: If the sprite doesn't work, this is why
    //scene_.subscene(subscene_).entity(entity_id()).set_mesh(mesh_id_); //Rebuild the entity
}

Sprite::~Sprite() {
    if(entity_) {
        subscene_.delete_entity(entity_->id());
        entity_ = nullptr;
    }
}

void Sprite::set_visible(bool value) {
    entity_->set_visible(value);
}

void Sprite::add_animation(const std::string& anim_name, const std::vector<TextureID>& frames, double duration) {
    /*
     * We need to hold MaterialPtr's directly in animations_ to maintain the ref-count as only one material
     * is bound at a time
     */
    MaterialID new_material_id;
    {
        //RAII
        MaterialPtr default_material = subscene_.material(subscene_.scene().default_material_id()).lock();
        new_material_id = default_material->clone();
    }
    MaterialPtr mat = subscene_.material(new_material_id).lock();
    mat->technique().pass(0).set_animated_texture_unit(0, frames, duration);
    mat->technique().pass(0).set_blending(BLEND_ALPHA);
    animations_[anim_name] = mat;

    if(animations_.size() == 1 && current_animation_.empty()) {
        set_active_animation(anim_name);
    }
}

void Sprite::set_active_animation(const std::string &anim_name) {
    current_animation_ = anim_name;

    //FIXME: Use override_material_id on the subentity
    kglt::MeshPtr mesh = entity_->mesh().lock();

    mesh->submesh(
        mesh->submesh_ids()[0]
    ).set_material_id(animations_[anim_name]->id());
}

void Sprite::set_render_dimensions(float width, float height) {
    kglt::MeshPtr mesh = entity_->mesh().lock();
    kglt::procedural::mesh::rectangle(mesh, width, height);

    //FIXME: This shouldn't be necessary! Changing a mesh should signal
    //the entity to rebuild
    //subscene.entity(entity_id()).set_mesh(mesh_id_); //Rebuild the entity

    set_active_animation(current_animation_); //Re-set the current animation
}

void Sprite::move_to(float x, float y, float z) {
    entity_->move_to(x, y, z);
}

void Sprite::rotate_to(float angle, float x, float y, float z) {
    entity_->rotate_to(angle, x, y, z);
}

}
}
