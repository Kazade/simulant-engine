#include "sprite.h"

#include "../scene.h"
#include "../stage.h"
#include "../procedural/mesh.h"
#include "../shortcuts.h"
#include "../actor.h"

namespace kglt {
namespace extra {

Sprite::Sprite(StageRef stage, const std::string& image_path, const FrameSize &size):
    stage_(stage),
    frame_size_(size) {

    StagePtr ss = stage_.lock();

    actor_id_ = ss->new_actor(ss->new_mesh());

    Actor& actor = ss->actor(actor_id_);

    kglt::procedural::mesh::rectangle(
        actor.mesh().lock(), 1.0, 1.0
    );

    //FIXME: Entities should be connected to mesh->signal_changed() and update automatically
    actor.set_mesh(actor.mesh_id()); //Rebuild the actor

    //Load the image
    auto tex = ss->texture(ss->new_texture_from_file(image_path));
    image_width_ = tex->width();
    image_height_ = tex->height();

    //Hold a reference to the new material
    auto mat = ss->material(ss->scene().clone_default_material());
    material_id_ = mat->id();

    //Set the texture on the material
    mat->technique().pass(0).set_texture_unit(0, tex->id());
    mat->technique().pass(0).set_blending(BLEND_ALPHA);

    //Finally set the material on the mesh
    actor.mesh().lock()->set_material_id(material_id_);

    update_texture_coordinates();

    ss->window().signal_step().connect(std::bind(&Sprite::update, this, std::placeholders::_1));
}

Sprite::~Sprite() {
    if(StagePtr ss = stage_.lock()) {
        ss->delete_actor(actor_id_);
    }
}

void Sprite::update(double dt) {
    if(current_animation_.empty()) {
        return;
    }

    auto it = animations_.find(current_animation_);
    assert(it != animations_.end());

    const Animation& anim = (*it).second;

    double change_per_second = double(anim.frames.end - anim.frames.start) / anim.duration;

    interp_ += change_per_second * dt;

    if(interp_ >= 1.0) {
        interp_ = 0.0;
        current_frame_ = next_frame_;
        next_frame_++;

        //FIXME: Add and handle loop=false
        if(next_frame_ >= anim.frames.end) {
            next_frame_ = anim.frames.start;
        }
        update_texture_coordinates();
    }
}

void Sprite::set_visible(bool value) {
    Actor& actor = stage_.lock()->actor(actor_id_);
    actor.set_visible(value);
}

void Sprite::add_animation(const std::string& anim_name, const FrameRange& frames, double duration) {
    StagePtr ss = stage_.lock();

    animations_.insert(std::make_pair(anim_name, Animation(duration, frames)));

    if(animations_.size() == 1 && current_animation_.empty()) {
        set_active_animation(anim_name);
    }
}

void Sprite::set_active_animation(const std::string &anim_name) {
    current_animation_ = anim_name;

    auto it = animations_.find(anim_name);
    assert(it != animations_.end());
    next_frame_ = (*it).second.frames.start;
}

void Sprite::set_render_dimensions(float width, float height) {
    Actor& actor = stage_.lock()->actor(actor_id_);

    kglt::MeshPtr mesh = actor.mesh().lock();

    //FIXME:
    //kglt::procedural::mesh::rectangle(mesh, width, height);

    //FIXME: This shouldn't be necessary! Changing a mesh should signal
    //the actor to rebuild
    //actor.set_mesh(actor.mesh_id()); //Rebuild the actor

    set_active_animation(current_animation_); //Re-set the current animation
}

void Sprite::move_to(float x, float y, float z) {
    stage_.lock()->actor(actor_id_).move_to(x, y, z);
}

void Sprite::rotate_to(float angle, float x, float y, float z) {
    stage_.lock()->actor(actor_id_).rotate_to(angle, x, y, z);
}

void Sprite::update_texture_coordinates() {
    uint8_t across = image_width_ / frame_size_.width;
    uint8_t down = image_height_ / frame_size_.height;

    double u = (1.0 / double(across)) * (current_frame_ % across);
    double v = (1.0 / double(down)) * (current_frame_ / across);

    Actor& actor = stage_.lock()->actor(actor_id_);
    kglt::MeshPtr mesh = actor.mesh().lock();

    mesh->shared_data().move_to_start();
    mesh->shared_data().tex_coord0(u, v);

    mesh->shared_data().move_next();
    mesh->shared_data().tex_coord0(u + (1.0 / double(across)) , v);

    mesh->shared_data().move_next();
    mesh->shared_data().tex_coord0(u + (1.0 / double(across)) , v + (1.0 / double(down)));

    mesh->shared_data().move_next();
    mesh->shared_data().tex_coord0(u, v + (1.0 / double(down)));

    mesh->shared_data().done();
}

}
}
