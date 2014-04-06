#include <functional>
#include "sprite.h"
#include "stage.h"
#include "actor.h"
#include "window_base.h"

using namespace kglt;

Sprite::Sprite(Stage *stage, SpriteID id):
    generic::Identifiable<SpriteID>(id),
    ParentSetterMixin<Object>(stage),
    Source(stage) {

    sprite_sheet_padding_ = std::make_pair(0, 0);

}

bool Sprite::init() {
    mesh_id_ = stage()->new_mesh_as_rectangle(1.0, 1.0);

    //Annoyingly, we can't use new_actor_with_parent_and_mesh here, because that looks
    //up our ID in the stage, which doesn't exist until this function returns. So instead
    //we make sure we are set as the parent on each update. Not ideal, but still.
    actor_id_ = stage()->new_actor_with_mesh(mesh_id_);

    return true;
}

void Sprite::cleanup() {
    stage()->delete_actor(actor_id_);
}

void Sprite::ask_owner_for_destruction() {
    stage()->delete_sprite(id());
}

void Sprite::update(double dt) {
    stage()->actor(actor_id_)->set_parent(id()); //Make sure every frame that our actor stays attached to us!

    if(animations_.empty()){
        return;
    }

    interp_ += dt;

    int diff = abs(current_animation_->frames.second - current_animation_->frames.first);
    if((diff && interp_ >= (current_animation_duration_ / double(diff))) || diff == 0) {
        interp_ = 0.0;
        current_frame_ = next_frame_;
        next_frame_++;

        //FIXME: Add and handle loop=false
        if(next_frame_ > current_animation_->frames.second) {
            if(next_animation_) {
                current_animation_ = next_animation_;
                current_animation_duration_ = current_animation_->duration;
                next_animation_ = nullptr;
            }
            next_frame_ = current_animation_->frames.first;
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
        current_animation_ = &anim;
        current_animation_duration_ = current_animation_->duration;
        current_frame_ = current_animation_->frames.first;
        next_frame_ = current_frame_ + 1;
        if(next_frame_ > current_animation_->frames.second) {
            next_frame_ = current_frame_;
        }
        interp_ = 0.0;
        update_texture_coordinates();
    }
}

void Sprite::queue_next_animation(const unicode &name) {
    auto it = animations_.find(name);
    if(it == animations_.end()) {
        throw DoesNotExist<Animation>();
    }

    next_animation_ = &(*it).second;
}

void Sprite::override_playing_animation_duration(const float new_duration) {
    current_animation_duration_ = new_duration;
}

void Sprite::play_animation(const unicode &name) {
    auto it = animations_.find(name);
    if(it == animations_.end()) {
        throw DoesNotExist<Animation>();
    }

    if(current_animation_ == &(*it).second) {
        return;
    }

    current_animation_ = &(*it).second;
    current_animation_duration_ = current_animation_->duration;
    next_animation_ = nullptr; //Wipe out the next animation, otherwise we'll get unexpected behaviour

    //Set the current frame and next frame appropriately
    current_frame_ = current_animation_->frames.first;
    next_frame_ = current_frame_ + 1;
    if(next_frame_ > current_animation_->frames.second) {
        //Handle the case where the animation is just a single frame
        next_frame_ = current_frame_;
    }
    interp_ = 0.0;
    update_texture_coordinates();
}

void Sprite::add_sequence(const unicode &name, const std::vector<AnimationSequenceStage> &stages) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void Sprite::play_sequence(const unicode &name) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void Sprite::flip_horizontally(bool value) {
    if(value == flipped_horizontally_) return;

    flipped_horizontally_ = value;
    update_texture_coordinates();
}

void Sprite::flip_vertically(bool value) {
    if(value == flipped_vertically_) return;

    flipped_vertically_ = value;
    update_texture_coordinates();
}

void Sprite::update_texture_coordinates() {
    uint8_t across = image_width_ / frame_width_;

    int x = current_frame_ % across;
    int y = current_frame_ / across;

    float x0 = sprite_sheet_margin_ + (x * (sprite_sheet_spacing_ + frame_width_)) + sprite_sheet_padding_.first;
    float x1 = x0 + (frame_width_ - sprite_sheet_padding_.first);
    float y0 = sprite_sheet_margin_ + (y * (sprite_sheet_spacing_ + frame_height_)) + sprite_sheet_padding_.second;
    float y1 = y0 + (frame_height_ - sprite_sheet_padding_.second);

    x0 = x0 / float(image_width_);
    x1 = x1 / float(image_width_);
    y0 = y0 / float(image_height_);
    y1 = y1 / float(image_height_);

    x0 += 0.5 / image_width_;
    x1 -= 0.5 / image_width_;

    y0 += 0.5 / image_height_;
    y1 -= 0.5 / image_height_;

    if(flipped_horizontally_) {
        std::swap(x0, x1);
    }

    if(flipped_vertically_) {
        std::swap(y0, y1);
    }

    {
        auto mesh = stage()->mesh(mesh_id_);

        mesh->shared_data().move_to_start();
        mesh->shared_data().tex_coord0(x0, y0);

        mesh->shared_data().move_next();
        mesh->shared_data().tex_coord0(x1, y0);

        mesh->shared_data().move_next();
        mesh->shared_data().tex_coord0(x1, y1);

        mesh->shared_data().move_next();
        mesh->shared_data().tex_coord0(x0, y1);

        mesh->shared_data().done();
    }
}

void Sprite::set_spritesheet(TextureID texture_id, uint32_t frame_width,
    uint32_t frame_height, uint32_t margin, uint32_t spacing,
    std::pair<uint32_t, uint32_t> padding
) {

    frame_width_ = frame_width;
    frame_height_ = frame_height;
    sprite_sheet_margin_ = margin;
    sprite_sheet_spacing_ = spacing;
    sprite_sheet_padding_ = padding;

    image_width_ = stage()->texture(texture_id)->width();
    image_height_ = stage()->texture(texture_id)->height();

    //Hold a reference to the new material
    material_id_ = stage()->scene().clone_default_material();

    stage()->mesh(mesh_id_)->set_material_id(material_id_);
    stage()->material(material_id_)->technique().pass(0).set_texture_unit(0, texture_id);
    stage()->material(material_id_)->technique().pass(0).set_blending(BLEND_ALPHA);

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
    auto mesh = stage()->mesh(mesh_id_);

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
