#include "sprite.h"

#include "../scene.h"
#include "../stage.h"
#include "../procedural/mesh.h"
#include "../shortcuts.h"
#include "../actor.h"

namespace kglt {
namespace extra {

Sprite::Sprite(Scene &scene, StageID stage_id, const std::string& image_path, const FrameSize &size):
    MoveableActorHolder(scene),
    image_path_(image_path),
    stage_id_(stage_id),
    frame_size_(size) {


}

bool Sprite::init() {
    actor_id_ = stage()->new_actor(stage()->new_mesh());

    kglt::procedural::mesh::rectangle(
        actor()->mesh(), 1.0, 1.0
    );

    //Load the image
    kglt::TextureID tex_id = stage()->new_texture_from_file(image_path_);
    {
        auto tex = stage()->texture(tex_id);

        image_width_ = tex->width();
        image_height_ = tex->height();

        //Hold a reference to the new material
        material_id_ = create_material_from_texture(stage()->scene(), tex_id);
    }


    {
        auto mat = stage()->material(material_id_);
        //Set the texture on the material
        mat->technique().pass(0).set_blending(BLEND_ALPHA);

        //Finally set the material on the mesh
        actor()->mesh()->set_material_id(material_id_);
    }

    update_texture_coordinates();

    //FIXME: Entities should be connected to mesh->signal_changed() and update automatically
    actor()->set_mesh(actor()->mesh_id()); //Rebuild the actor

    stage()->window().signal_step().connect(std::bind(&Sprite::update, this, std::placeholders::_1));
    return true;
}

Sprite::~Sprite() {
    if(actor_id_) {
        stage()->delete_actor(actor_id_);
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
    actor()->set_visible(value);
}

void Sprite::add_animation(const std::string& anim_name, const FrameRange& frames, double duration) {
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
    {
        auto mesh = actor()->mesh();

        mesh->shared_data().move_to_start();
        mesh->shared_data().position((-width / 2.0), (-height / 2.0), 0);

        mesh->shared_data().move_next();
        mesh->shared_data().position((width / 2.0), (-height / 2.0), 0);

        mesh->shared_data().move_next();
        mesh->shared_data().position((width / 2.0),  (height / 2.0), 0);

        mesh->shared_data().move_next();
        mesh->shared_data().position((-width / 2.0),  (height / 2.0), 0);

        mesh->shared_data().done();
    }

    //FIXME: This shouldn't be necessary! Changing a mesh should signal
    //the actor to rebuild
    actor()->set_mesh(actor()->mesh_id()); //Rebuild the actor

    set_active_animation(current_animation_); //Re-set the current animation
}

void Sprite::move_to(float x, float y, float z) {
    actor()->set_absolute_position(x, y, z);
}

void Sprite::rotate_to(const Degrees &angle, float x, float y, float z) {
    actor()->set_absolute_rotation(angle, x, y, z);
}

void Sprite::update_texture_coordinates() {
    uint8_t across = image_width_ / frame_size_.width;
    uint8_t down = image_height_ / frame_size_.height;

    double u = (1.0 / double(across)) * (current_frame_ % across);
    double v = (1.0 / double(down)) * (current_frame_ / across);

    {
        auto mesh = actor()->mesh();

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
}
