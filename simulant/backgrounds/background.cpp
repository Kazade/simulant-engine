//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../stage.h"
#include "../nodes/camera.h"

#include "background.h"


namespace smlt {

Background::Background(BackgroundID background_id, BackgroundManager *manager, BackgroundType type):
    generic::Identifiable<BackgroundID>(background_id),
    manager_(manager),
    type_(type) {

}

bool Background::init() {
    //Create a stage to add to the render pipeline
    stage_ = manager_->window->new_stage(PARTITIONER_NULL).fetch();
    //We need to create an orthographic camera
    camera_id_ = stage_->new_camera();

    // Create a unit projection
    // FIXME: allow passing a viewport and adjust accordingly
    auto cam = camera_id_.fetch();
    cam->set_orthographic_projection(
        -0.5, 0.5, -0.5, 0.5
    );

    sprite_ = stage->sprites->new_sprite().fetch();

    //Add a pass for this background
    pipeline_id_ = manager_->window->render(stage_, cam).with_priority(
        smlt::RENDER_PRIORITY_BACKGROUND + manager_->background_count()
    );

    return true;
}

void Background::cleanup() {
    //Remove the pipeline and delete the stage, everything else is cleaned
    //up automatically
    manager_->window->delete_pipeline(pipeline_id_);
    manager_->window->delete_stage(stage_->id());
}

void Background::update(float dt) {
    if(type_ != BACKGROUND_TYPE_SCROLL) {
        return;
    }

    auto mat = sprite_->material_id().fetch();
    auto pass = mat->first_pass();
    if(pass->texture_unit_count()) {
        pass->texture_unit(0).scroll_x(x_rate_ * dt);
        pass->texture_unit(0).scroll_y(y_rate_ * dt);
    }
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

    auto mat = sprite_->material_id().fetch();
    mat->first_pass()->set_depth_write_enabled(false);
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

    auto mat = sprite_->material_id().fetch();
    mat->first_pass()->set_depth_write_enabled(false);
}

void Background::set_resize_style(BackgroundResizeStyle style) {
    style_ = style;
}

void Background::ask_owner_for_destruction() {
    manager_->delete_background(id());
}


}
