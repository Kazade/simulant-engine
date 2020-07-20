#include "splash.h"

#include "../stage.h"
#include "../compositor.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../core.h"
#include "../platform.h"
#include "../nodes/ui/ui_manager.h"
#include "../nodes/ui/label.h"

#include "splash.h"

#define SIMULANT_TEXT_512 "simulant/textures/simulant-text-512.png"
#define SIMULANT_TEXT_1024 "simulant/textures/simulant-text.png"

#define SIMULANT_LOGO_256 "simulant/textures/simulant-icon-256.png"
#define SIMULANT_LOGO_512 "simulant/textures/simulant-icon.png"

namespace smlt {
namespace scenes {

void Splash::load() {
    //Create a stage
    stage_ = core->new_stage();

    auto text_file = (core->width() < 1200) ? SIMULANT_TEXT_512 : SIMULANT_TEXT_1024;
    auto logo_file = (core->width() < 1200) ? SIMULANT_LOGO_256 : SIMULANT_LOGO_512;

    auto text_texture = stage_->assets->new_texture_from_file(text_file);
    text_ = stage_->ui->new_widget_as_image(text_texture);

    auto texture = stage_->assets->new_texture_from_file(logo_file);
    image_ = stage_->ui->new_widget_as_image(texture);

    sound_ = core->shared_assets->new_sound_from_file("simulant/sounds/simulant.wav", smlt::GARBAGE_COLLECT_NEVER);

    image_->set_anchor_point(0.5f, 0.0f);
    image_->set_opacity(0.0f);

    text_->set_anchor_point(0.5, 1.0);

    image_->move_to(core->coordinate_from_normalized(0.5f, 0.4f));
    text_->move_to(core->coordinate_from_normalized(0.5f, 0.4f));

    //Create an orthographic camera
    camera_ = stage_->new_camera();

    camera_->set_orthographic_projection(
        0, core->width(), 0, core->height()
    );

    //Create an inactive pipeline
    pipeline_ = compositor->render(stage_, camera_);
    link_pipeline(pipeline_);
}

void Splash::unload() {
    //Clean up
    pipeline_->destroy();
    core->destroy_stage(stage_->id());
    core->shared_assets->destroy_sound(sound_);
}

void Splash::activate() {
    start_time_ = core->time_keeper->now_in_us();
    core->play_sound(sound_);
}

void Splash::deactivate() {
    //Deactivate the Splash pipeline
    core->idle->remove(connection_);
}

void Splash::update(float dt) {
    /* We can't use dt as we load the scene in the main thread, and
     * by the time we hit update dt will be large and we'll
     * we'll immediately start loading the next scene */
    _S_UNUSED(dt);

    const float FADE_SPEED = 2.0f;

    if(!this->is_active()) {
        return;
    }

    auto diff = core->time_keeper->now_in_us() - start_time_;
    float elapsed = float(diff) / 1000000.0f;

    image_->set_opacity(std::min(elapsed * FADE_SPEED, 1.0f));

    if(elapsed >= time_) {
        scenes->activate(target_);
    }
}

}
}
