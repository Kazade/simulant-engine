#include <functional>
#include "sprite.h"
#include "stage.h"
#include "actor.h"
#include "window_base.h"
#include "animation.h"

using namespace kglt;

Sprite::Sprite(SpriteID id, Stage *stage):
    generic::Identifiable<SpriteID>(id),
    ParentSetterMixin<MoveableObject>(stage),
    Source(stage) {

    sprite_sheet_padding_ = std::make_pair(0, 0);

    using namespace std::placeholders;

    animation_state_ = std::make_shared<KeyFrameAnimationState>(
        shared_from_this(),
        std::bind(&Sprite::refresh_animation_state, this, _1, _2, _3)
    );
}

bool Sprite::init() {
    mesh_id_ = stage->assets->new_mesh_as_rectangle(1.0, 1.0);

    //Annoyingly, we can't use new_actor_with_parent_and_mesh here, because that looks
    //up our ID in the stage, which doesn't exist until this function returns. So instead
    //we make sure we are set as the parent on each update. Not ideal, but still.
    actor_id_ = stage->new_actor_with_mesh(mesh_id_);

    return true;
}

void Sprite::cleanup() {
    if(actor_id_) {
        stage->delete_actor(actor_id_);
    }
}

void Sprite::ask_owner_for_destruction() {
    stage->delete_sprite(id());
}

void Sprite::update(double dt) {
    stage->actor(actor_id_)->set_parent(id()); //Make sure every frame that our actor stays attached to us!

    // Update any keyframe animations
    animation_state_->update(dt);
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

    auto current_frame = animation_state_->current_frame();
    int x = current_frame % across;
    int y = current_frame / across;

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
        auto mesh = stage->assets->mesh(mesh_id_);

        mesh->shared_data->move_to_start();
        mesh->shared_data->tex_coord0(x0, y0);

        mesh->shared_data->move_next();
        mesh->shared_data->tex_coord0(x1, y0);

        mesh->shared_data->move_next();
        mesh->shared_data->tex_coord0(x1, y1);

        mesh->shared_data->move_next();
        mesh->shared_data->tex_coord0(x0, y1);

        mesh->shared_data->done();
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

    image_width_ = stage->assets->texture(texture_id)->width();
    image_height_ = stage->assets->texture(texture_id)->height();

    //Hold a reference to the new material
    material_id_ = stage->assets->new_material_from_texture(texture_id);
    stage->assets->mesh(mesh_id_)->set_material_id(material_id_);

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
        throw std::runtime_error("You can't call set_render_dimensions without first specifying a spritesheet");
    }

    if(width < 0 && height > 0) {
        //Determine aspect ratio to calculate width
        width = height * (float(frame_width_) / float(frame_height_));
    } else if(width > 0 && height < 0) {
        //Determine aspect ratio to calculate height
        height = width / (float(frame_width_) / float(frame_height_));
    } else if(width < 0 && height < 0) {
        L_ERROR("You must specify a positive value for width or height, or both");
        width = std::max(width, 0.0f);
        height = std::max(height, 0.0f);
    }

    render_width_ = width;
    render_height_ = height;

    //Rebuild the mesh
    auto mesh = stage->assets->mesh(mesh_id_);

    mesh->shared_data->move_to_start();
    mesh->shared_data->position((-width / 2.0), (-height / 2.0), 0);

    mesh->shared_data->move_next();
    mesh->shared_data->position((width / 2.0), (-height / 2.0), 0);

    mesh->shared_data->move_next();
    mesh->shared_data->position((width / 2.0),  (height / 2.0), 0);

    mesh->shared_data->move_next();
    mesh->shared_data->position((-width / 2.0),  (height / 2.0), 0);

    mesh->shared_data->done();
}
