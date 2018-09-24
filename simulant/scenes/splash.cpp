#include "splash.h"

#include "../stage.h"
#include "../render_sequence.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../window.h"
#include "../platform.h"
#include "../nodes/ui/ui_manager.h"
#include "../nodes/ui/label.h"

#include "splash.h"

namespace smlt {
namespace scenes {

void Splash::load() {
    //Create a stage
    stage_ = window->new_stage();

    auto text_texture = stage_->assets->new_texture_from_file("simulant/textures/simulant-text.png");
    text_ = stage_->ui->new_widget_as_image(text_texture);
    text_->move_to(window->coordinate_from_normalized(0.53, 0.5));

    auto texture = stage_->assets->new_texture_from_file("simulant/textures/simulant-icon.png");
    image_ = stage_->ui->new_widget_as_image(texture);
    image_->move_to(window->coordinate_from_normalized(0.25, 0.5));

    /* Scale for window resolution */
    float scale = 0.5 * (window->height() / 720.0f);

    image_->set_width(image_->width() * scale);
    image_->set_height(image_->height() * scale);

    text_->set_width(text_->width() * scale);
    text_->set_height(text_->height() * scale);

    //Create an orthographic camera
    camera_ = stage_->new_camera();

    camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    //Create an inactive pipeline
    pipeline_ = window->render(stage_, camera_);
    pipeline_->deactivate();
}

void Splash::unload() {
    //Clean up
    pipeline_->deactivate();
    window->delete_pipeline(pipeline_->id());
    window->delete_stage(stage_->id());
}

void Splash::activate() {
    pipeline_->activate();
}

void Splash::deactivate() {
    //Deactivate the Splash pipeline
    pipeline_->deactivate();
}

void Splash::update(float dt) {
    timer_ += dt;
    if(timer_ > time_) {
        scenes->activate(target_);
    }
}

}
}
