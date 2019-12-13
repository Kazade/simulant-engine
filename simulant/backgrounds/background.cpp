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

#include "../stage.h"
#include "../nodes/camera.h"

#include "background.h"
#include "../render_sequence.h"
#include "background_manager.h"
#include "../window.h"

namespace smlt {

Background::Background(BackgroundID background_id, BackgroundManager *manager, BackgroundType type):
    generic::Identifiable<BackgroundID>(background_id),
    manager_(manager),
    type_(type) {

}

bool Background::init() {
    //Create a stage to add to the render pipeline
    stage_ = manager_->window->new_stage(PARTITIONER_NULL);
    //We need to create an orthographic camera
    camera_ = stage_->new_camera();

    // Create a unit projection
    // FIXME: allow passing a viewport and adjust accordingly
    camera_->set_orthographic_projection(
        -0.5, 0.5, -0.5, 0.5
    );

    sprite_ = stage->sprites->new_sprite();

    //Add a pass for this background
    pipeline_ = manager_->window->render(stage_, camera_).with_priority(
        smlt::RENDER_PRIORITY_BACKGROUND + manager_->background_count()
    );

    pipeline_->activate();

    return true;
}

void Background::clean_up() {
    //Remove the pipeline and delete the stage, everything else is cleaned
    //up automatically
    manager_->window->destroy_pipeline(pipeline_->id());
    manager_->window->destroy_stage(stage_->id());
}

void Background::update(float dt) {
    if(type_ != BACKGROUND_TYPE_SCROLL) {
        return;
    }

    assert(sprite_);

    auto mat = sprite_->material_id().fetch();
    assert(mat);

    auto diffuse_map = mat->diffuse_map();
    diffuse_map.scroll_x(x_rate_ * dt);
    diffuse_map.scroll_y(y_rate_ * dt);
}

void Background::set_horizontal_scroll_rate(float x_rate) {
    x_rate_ = x_rate;
}

void Background::set_vertical_scroll_rate(float y_rate) {
    y_rate_ = y_rate;
}

void Background::set_spritesheet(TextureID texture_id, float frame_width, float frame_height, SpritesheetAttrs attrs) {
    if(type_ != BACKGROUND_TYPE_ANIMATED) {
        L_WARN("Called Background::set_spritesheet on an scrollable background, use set_texture instead.");
        return;
    }

    // Ensure the texture is from the same stage, or from shared_assets
    auto tex = stage->assets->texture(texture_id);
    assert(tex);

    sprite_->set_spritesheet(texture_id, frame_width, frame_height, attrs);
    sprite_->set_render_dimensions(
        frame_width / frame_height,
        1.0f
    );

    auto mat = sprite_->material_id().fetch();
    mat->set_depth_write_enabled(false);
    mat->set_blend_func(smlt::BLEND_NONE);
}

void Background::set_texture(TextureID texture_id) {
    if(type_ != BACKGROUND_TYPE_SCROLL) {
        L_WARN("Called Background::set_texture on an animated background, use set_spritesheet instead.");
        return;
    }

    // Ensure the texture is from the same stage, or from shared_assets
    auto tex = stage_->assets->texture(texture_id);
    assert(tex);

    sprite_->set_spritesheet(
        tex->id(),
        tex->width(), tex->height()
    );
    sprite_->set_render_dimensions(
        tex->width() / tex->height(),
        1.0f
    );

    auto mat = sprite_->material_id().fetch();
    mat->set_depth_write_enabled(false);
    mat->set_blend_func(smlt::BLEND_NONE);
}

void Background::set_resize_style(BackgroundResizeStyle style) {
    style_ = style;
}

void Background::destroy() {
    manager_->destroy_background(id());
}


}
