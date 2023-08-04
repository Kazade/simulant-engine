//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <functional>
#include "sprite.h"
#include "actor.h"

#include "../stage.h"
#include "../window.h"
#include "../animation.h"
#include "sprites/sprite_manager.h"

using namespace smlt;

Sprite::Sprite(SpriteManager *manager, SoundDriver* sound_driver):
    ContainerNode(manager->stage.get(), STAGE_NODE_TYPE_SPRITE),
    AudioSource(manager->stage, this, sound_driver),
    manager_(manager) {

    sprite_sheet_padding_ = std::make_pair(0, 0);
}

bool Sprite::on_destroy() {
    manager_->destroy_sprite(id());
    return true;
}

bool Sprite::on_destroy_immediately() {
    manager_->sprite_manager_->destroy_immediately(id());
    return true;
}

bool Sprite::init() {
    auto mesh = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
    mesh->new_submesh_as_rectangle("sprite", stage->assets->new_material(), 1.0, 1.0f);

    mesh_ = mesh;

    //Annoyingly, we can't use new_actor_with_parent_and_mesh here, because that looks
    //up our ID in the stage, which doesn't exist until this function returns
    actor_ = stage->new_actor_with_mesh(mesh_);
    actor_->set_parent(this);

    actor_id_ = actor_->id();

    set_render_dimensions(1.0f, 1.0f);

    using namespace std::placeholders;
    animation_state_ = std::make_shared<KeyFrameAnimationState>(
        this,
        std::bind(&Sprite::refresh_animation_state, this, _1, _2, _3)
    );

    return true;
}

void Sprite::clean_up() {
    if(actor_ && stage->has_actor(actor_id_)) {
        stage->destroy_actor(actor_id_);
        actor_ = nullptr;
        actor_id_ = StageNodeID();
    }

    StageNode::clean_up();
}

void Sprite::on_update(float dt) {
    if(actor_) {
        actor_->set_parent(this); //Make sure every frame that our actor stays attached to us!
    }

    // Update any keyframe animations
    animation_state_->update(dt);
}


void Sprite::flip_horizontally(bool value) {
    if(value == flipped_horizontally_) return;

    flipped_horizontally_ = value;
    update_texture_coordinates();
}

const AABB& Sprite::aabb() const {
    static const smlt::AABB IDENTITY;

    if(!actor_) {
        return IDENTITY;
    }

    return actor_->aabb();
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

    x0 += 0.5f / image_width_;
    x1 -= 0.5f / image_width_;

    y0 += 0.5f / image_height_;
    y1 -= 0.5f / image_height_;

    if(flipped_horizontally_) {
        std::swap(x0, x1);
    }

    if(flipped_vertically_) {
        std::swap(y0, y1);
    }

    {
        mesh_->vertex_data->move_to_start();
        mesh_->vertex_data->tex_coord0(x0, y0);

        mesh_->vertex_data->move_next();
        mesh_->vertex_data->tex_coord0(x1, y0);

        mesh_->vertex_data->move_next();
        mesh_->vertex_data->tex_coord0(x1, y1);

        mesh_->vertex_data->move_next();
        mesh_->vertex_data->tex_coord0(x0, y1);

        mesh_->vertex_data->done();
    }
}

void Sprite::set_spritesheet(TexturePtr texture, uint32_t frame_width, uint32_t frame_height, SpritesheetAttrs attrs) {
    frame_width_ = frame_width;
    frame_height_ = frame_height;
    sprite_sheet_margin_ = attrs.margin;
    sprite_sheet_spacing_ = attrs.spacing;
    sprite_sheet_padding_ = std::make_pair(attrs.padding_horizontal, attrs.padding_vertical);

    image_width_ = texture->width();
    image_height_ = texture->height();

    //Hold a reference to the new material
    auto mat = stage->assets->new_material_from_texture(texture);
    mat->set_blend_func(smlt::BLEND_ALPHA);

    material_ = mat;
    mesh_->set_material(mat);

    update_texture_coordinates();
}

void Sprite::set_render_dimensions_from_height(float height) {
    set_render_dimensions(-1, height);
}

void Sprite::set_render_priority(RenderPriority priority) {
    actor_->set_render_priority(priority);
}

void Sprite::set_alpha(float alpha) {
    alpha_ = alpha;
    mesh_->set_diffuse(smlt::Colour(1.0f, 1.0f, 1.0f, alpha_));
}

void Sprite::set_render_dimensions_from_width(float width) {
    set_render_dimensions(width, -1);
}

void Sprite::set_render_dimensions(float width, float height) {
    if((!frame_width_ || !frame_height_) && (width < 0 || height < 0)) {
        throw std::runtime_error("You can't call set_render_dimensions without first specifying a spritesheet");
    }

    if(width < 0 && height > 0) {
        //Determine aspect ratio to calculate width
        width = height * (float(frame_width_) / float(frame_height_));
    } else if(width > 0 && height < 0) {
        //Determine aspect ratio to calculate height
        height = width / (float(frame_width_) / float(frame_height_));
    } else if(width < 0 && height < 0) {
        S_ERROR("You must specify a positive value for width or height, or both");
        width = std::max(width, 0.0f);
        height = std::max(height, 0.0f);
    }

    render_width_ = width;
    render_height_ = height;

    //Rebuild the mesh
    mesh_->vertex_data->move_to_start();
    mesh_->vertex_data->position((-width / 2.0f), (-height / 2.0f), 0);

    mesh_->vertex_data->move_next();
    mesh_->vertex_data->position((width / 2.0f), (-height / 2.0f), 0);

    mesh_->vertex_data->move_next();
    mesh_->vertex_data->position((width / 2.0f),  (height / 2.0f), 0);

    mesh_->vertex_data->move_next();
    mesh_->vertex_data->position((-width / 2.0f),  (height / 2.0f), 0);

    mesh_->vertex_data->done();
}
