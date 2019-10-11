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

    sound_ = window->shared_assets->new_sound_from_file("simulant/sounds/simulant.wav", smlt::GARBAGE_COLLECT_NEVER);

    /* Scale for window resolution */
    float scale = 0.5 * (window->height() / 720.0f);

    auto round = [](float x, float f) -> float {
        return f * ceil(x / f);
    };

    round(scale, 0.25);

    image_->set_width(image_->width() * scale * 0.75);
    image_->set_height(image_->height() * scale * 0.75);
    image_->set_anchor_point(0.5, 0.0);
    image_->set_opacity(0.0f);

    text_->set_width(text_->width() * scale);
    text_->set_height(text_->height() * scale);
    text_->set_anchor_point(0.5, 1.0);

    image_->move_to(window->coordinate_from_normalized(0.5, 0.5));
    text_->move_to(window->coordinate_from_normalized(0.5, 0.5));

    //Create an orthographic camera
    camera_ = stage_->new_camera();

    camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    //Create an inactive pipeline
    pipeline_ = window->render(stage_, camera_);
    link_pipeline(pipeline_);
    timer_ = 0.0f;
}

void Splash::unload() {
    //Clean up
    window->destroy_pipeline(pipeline_->id());
    window->destroy_stage(stage_->id());
    window->shared_assets->destroy_sound(sound_);
}

void Splash::activate() {
    /* Add gradual fade in */
    std::shared_ptr<float> opacity = std::make_shared<float>(0.0f);
    connection_ = window->idle->add_timeout(1.0 / 60, [opacity, this]() -> bool {
        const float SPEED = 2.0f;
        *opacity += window->time_keeper->delta_time() * SPEED;
        image_->set_opacity(std::min(*opacity, 1.0f));
        return *opacity < 1.0f;
    });

    window->play_sound(sound_);
}

void Splash::deactivate() {
    //Deactivate the Splash pipeline
    window->idle->remove(connection_);
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
