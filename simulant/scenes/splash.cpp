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

    auto texture = stage_->assets->new_texture_from_file("simulant/textures/simulant-icon.png");
    image_ = stage_->ui->new_widget_as_image(texture);

    /* Scale for window resolution */
    float scale = 0.5 * (window->height() / 720.0f);

    auto round = [](float x, float f) -> float {
        return f * ceil(x / f);
    };

    round(scale, 0.25);

    image_->set_width(image_->width() * scale);
    image_->set_height(image_->height() * scale);

    text_->set_width(text_->width() * scale);
    text_->set_height(text_->height() * scale);


    /* We have the final dimensions so let's work out what the widths should be */

    float gap = image_->width() / 4.0f; // Leave a gap between the icon and the text

    float total = text_->width() + image_->width() + gap;

    float xpos = ((window->width() - total) / 2.0f) + (image_->width() / 2);

    auto ypos = window->coordinate_from_normalized(0, 0.5).y;

    image_->move_to(xpos, ypos);
    text_->move_to(xpos + image_->width() + gap, ypos);

    //Create an orthographic camera
    camera_ = stage_->new_camera();

    camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    //Create an inactive pipeline
    pipeline_ = window->render(stage_, camera_);
    pipeline_->deactivate();
    timer_ = 0.0f;
}

void Splash::unload() {
    //Clean up
    pipeline_->deactivate();
    window->destroy_pipeline(pipeline_->id());
    window->destroy_stage(stage_->id());
}

void Splash::activate() {
    pipeline_->activate();
}

void Splash::deactivate() {
    //Deactivate the Splash pipeline
    pipeline_->deactivate();
}

void Splash::update(float dt) {
    if(!this->is_active()) {
        return;
    }

    timer_ += dt;
    if(timer_ >= time_) {
        scenes->activate(target_);
    }
}

}
}
