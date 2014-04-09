#include "stage.h"
#include "background.h"
#include "window_base.h"
#include "camera.h"
#include "render_sequence.h"
#include "actor.h"

namespace kglt {

Background::Background(Scene *scene, BackgroundID background_id):
    generic::Identifiable<BackgroundID>(background_id),
    scene_(scene) {

}

void Background::update_camera() {
    //FIXME: Viewport should probably be overridable
    float vp_width = scene_->window().viewport()->width();
    float vp_height = scene_->window().viewport()->height();

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

    scene_->camera(camera_id_)->set_orthographic_projection(
        0,
        width,
        0,
        height,
        -1.0, 1.0
    );
}

bool Background::init() {
    //Create a stage to add to the render pipeline
    stage_id_ = scene_->new_stage(PARTITIONER_NULL);

    //We need to create an orthographic camera
    camera_id_ = scene_->new_camera();
    update_camera();

    //Add a pass for this background
    //FIXME: priority = -1000
    pipeline_id_ = scene_->window().render_sequence()->new_pipeline(
        stage_id_,
        camera_id_,
        ViewportID(), //FIXME: Should be overridable
        TextureID(),
        -100
    );

    actor_id_ = scene_->stage(stage_id_)->new_actor();
    //Load the background material
    material_id_ = scene_->stage(stage_id_)->new_material_from_file("kglt/materials/background.kglm");

    auto mesh = scene_->stage(stage_id_)->new_mesh_as_rectangle(1, 1, Vec2(0.5, 0.5));
    scene_->stage(stage_id_)->actor(actor_id_)->set_mesh(mesh);
    scene_->stage(stage_id_)->mesh(mesh)->set_material_id(material_id_);

    scene_->stage(stage_id_)->material(material_id_)->technique().pass(0).set_blending(BLEND_ALPHA);
    scene_->stage(stage_id_)->material(material_id_)->technique().pass(0).set_depth_test_enabled(false);
    scene_->stage(stage_id_)->material(material_id_)->technique().pass(0).set_depth_write_enabled(false);
    return true;
}

void Background::cleanup() {
    //Remove the pipeline and delete the camera, everything else is cleaned
    //up automatically when the node is detached from the scene tree
    scene_->window().render_sequence()->delete_pipeline(pipeline_id_);
    scene_->delete_camera(camera_id_);
}

void Background::update(double dt) {
    scene_->stage(stage_id_)->material(material_id_)->technique().pass(0).texture_unit(0).scroll_x(x_rate_ * dt);
    scene_->stage(stage_id_)->material(material_id_)->technique().pass(0).texture_unit(0).scroll_y(y_rate_ * dt);
}

void Background::set_horizontal_scroll_rate(float x_rate) {
    x_rate_ = x_rate;
}

void Background::set_vertical_scroll_rate(float y_rate) {
    y_rate_ = y_rate;
}

void Background::set_texture(TextureID tex) {
    scene_->stage(stage_id_)->material(material_id_)->technique().pass(0).set_texture_unit(0, tex);
}

void Background::set_resize_style(BackgroundResizeStyle style) {
    style_ = style;
}

void Background::ask_owner_for_destruction() {
    scene_->delete_background(id());
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
