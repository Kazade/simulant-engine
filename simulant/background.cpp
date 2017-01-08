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

#include "stage.h"
#include "background.h"
#include "window_base.h"
#include "camera.h"
#include "render_sequence.h"
#include "actor.h"

namespace smlt {

Background::Background(BackgroundID background_id, BackgroundManager *manager):
    generic::Identifiable<BackgroundID>(background_id),
    manager_(manager) {

}

void Background::update_camera(const Viewport &viewport) {
    float vp_width = viewport.width();
    float vp_height = viewport.height();

    float width, height;
    if(style_ == BACKGROUND_RESIZE_ZOOM) {
        if(vp_width > vp_height) {
            width = 1.0;
            height = float(vp_height) / float(vp_width);
        } else {
            height = 1.0;
            width = float(vp_width) / float(vp_height);
        }
    } else {
        width = 1.0;
        height = 1.0;
    }

    manager_->window->camera(camera_id_)->set_orthographic_projection(
        0,
        width,
        0,
        height,
        -1.0, 1.0
    );
}

bool Background::init() {
    //We need to create an orthographic camera
    camera_id_ = manager_->window->new_camera();
    update_camera(Viewport()); //FIXME: Only fullscreen??

    //Create a stage to add to the render pipeline
    stage_id_ = manager_->window->new_stage(PARTITIONER_NULL);

    {
        auto stage = manager_->window->stage(stage_id_);

        actor_id_ = stage->new_actor();
        //Load the background material
        material_id_ = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);

        auto mesh = stage->assets->new_mesh_as_rectangle(1, 1, Vec2(0.5, 0.5));
        stage->actor(actor_id_)->set_mesh(mesh);
        stage->assets->mesh(mesh)->set_material_id(material_id_);
    }

    //Add a pass for this background
    pipeline_id_ = manager_->window->render(stage_id_, camera_id_).with_priority(
        smlt::RENDER_PRIORITY_BACKGROUND + manager_->background_count()
    );

    return true;
}

void Background::cleanup() {
    //Remove the pipeline and delete the camera, everything else is cleaned
    //up automatically when the node is detached from the scene tree
    manager_->window->delete_pipeline(pipeline_id_);
    manager_->window->delete_camera(camera_id_);
}

void Background::update(double dt) {
    auto pass = manager_->window->stage(stage_id_)->assets->material(material_id_)->first_pass();
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

void Background::set_texture(TextureID tex) {
    manager_->window->stage(stage_id_)->assets->material(material_id_)->pass(0)->set_texture_unit(0, tex);
}

void Background::set_resize_style(BackgroundResizeStyle style) {
    style_ = style;
}

void Background::ask_owner_for_destruction() {
    manager_->window->delete_background(id());
}

unicode Background::__unicode__() const {
    if(has_name()) {
        return name();
    } else {
        return _u("Background {0}").format(this->id());
    }
}

const bool Background::has_name() const {
    return !name_.empty();
}

void Background::set_name(const unicode &name) {
    name_ = name;
}

const unicode Background::name() const {
    return name_;
}

}
