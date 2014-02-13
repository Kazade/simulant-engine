#include "sprite.h"
#include "stage.h"
#include "actor.h"
#include "window_base.h"

using namespace kglt;

Sprite::Sprite(Stage *stage, SpriteID id):
    generic::Identifiable<SpriteID>(id),
    ParentSetterMixin<Object>(stage),
    Source(stage) {
}

bool Sprite::init() {
    mesh_id_ = stage().new_mesh_as_rectangle(1.0, 1.0);

    //Annoyingly, we can't use new_actor_with_parent_and_mesh here, because that looks
    //up our ID in the stage, which doesn't exist until this function returns. So instead
    //we make sure we are set as the parent on each update. Not ideal, but still.
    actor_id_ = stage().new_actor_with_mesh(mesh_id_);

    stage().window().signal_step().connect(std::bind(&Sprite::update, this, std::placeholders::_1));
    return true;
}

void Sprite::cleanup() {
    stage().delete_actor(actor_id_);
}

void Sprite::destroy() {
    stage().delete_sprite(id());
}

void Sprite::update(double dt) {
    stage().actor(actor_id_)->set_parent(id()); //Make sure every frame that our actor stays attached to us!

    if(animations_.empty()){
        return;
    }

    double change_per_second = double(fabs(current_animation_.frames.second - current_animation_.frames.first)) / current_animation_.duration;

    interp_ += change_per_second * dt;

    if(interp_ >= 1.0) {
        interp_ = 0.0;
        current_frame_ = next_frame_;
        next_frame_++;

        //FIXME: Add and handle loop=false
        if(next_frame_ >= current_animation_.frames.second) {
            next_frame_ = current_animation_.frames.first;
        }
        update_texture_coordinates();
    }
}

void Sprite::add_animation(const unicode &name, uint32_t start_frame, uint32_t end_frame, float duration) {
    auto& anim = animations_[name];
    anim.frames.first = start_frame;
    anim.frames.second = end_frame;
    anim.duration = duration;

    if(animations_.size() == 1) {
        current_animation_ = next_animation_ = anim;
        current_frame_ = current_animation_.frames.first;
        next_frame_ = current_frame_ + 1;
        if(next_frame_ >= current_animation_.frames.second) {
            next_frame_ = current_frame_;
        }
        update_texture_coordinates();
    }
}

void Sprite::set_next_animation(const unicode &name) {
    auto it = animations_.find(name);
    if(it == animations_.end()) {
        throw DoesNotExist<Animation>();
    }

    next_animation_ = (*it).second;
}

void Sprite::set_current_animation(const unicode &name) {
    auto it = animations_.find(name);
    if(it == animations_.end()) {
        throw DoesNotExist<Animation>();
    }

    current_animation_ = (*it).second;
    next_frame_ = current_animation_.frames.first;
}

void Sprite::update_texture_coordinates() {
    uint8_t across = image_width_ / frame_width_;
    uint8_t down = image_height_ / frame_height_;

    double u = (1.0 / double(across)) * (current_frame_ % across);
    double v = (1.0 / double(down)) * (current_frame_ / across);

    {
        auto mesh = stage().mesh(mesh_id_);

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

void Sprite::set_spritesheet(TextureID texture_id, uint32_t frame_width, uint32_t frame_height, uint32_t margin, uint32_t spacing) {
    frame_width_ = frame_width;
    frame_height_ = frame_height;
    sprite_sheet_margin_ = margin;
    sprite_sheet_spacing_ = spacing;

    image_width_ = stage().texture(texture_id)->width();
    image_height_ = stage().texture(texture_id)->height();

    //Hold a reference to the new material
    material_id_ = stage().scene().clone_default_material();

    stage().mesh(mesh_id_)->set_material_id(material_id_);
    stage().material(material_id_)->technique().pass(0).set_texture_unit(0, texture_id);
    stage().material(material_id_)->technique().pass(0).set_blending(BLEND_ALPHA);

    update_texture_coordinates();
}

void Sprite::set_render_dimensions_from_height(float height) {
    set_render_dimensions(-1, height);
}

void Sprite::set_render_dimensions_from_width(float width) {
    set_render_dimensions(width, -1);
}

void Sprite::set_render_dimensions(float width, float height) {
    if(!frame_width_ || !frame_height_) {
        throw LogicError("You can't call set_render_dimensions without first specifying a spritesheet");
    }

    if(width < 0 && height > 0) {
        //Determine aspect ratio to calculate width
        width = height * (float(frame_width_) / float(frame_height_));
    } else if(width > 0 && height < 0) {
        //Determine aspect ratio to calculate height
        height = width / (float(frame_width_) / float(frame_height_));
    } else if(width < 0 && height < 0) {
        throw ValueError("You must specify a positive value for width or height, or both");
    }

    render_width_ = width;
    render_height_ = height;

    //Rebuild the mesh
    auto mesh = stage().mesh(mesh_id_);

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
