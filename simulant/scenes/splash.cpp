#include "splash.h"

#include "../stage.h"
#include "../compositor.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../window.h"
#include "../platform.h"
#include "../nodes/ui/ui_manager.h"
#include "../nodes/ui/label.h"
#include "../application.h"
#include "../time_keeper.h"
#include "../scenes/scene_manager.h"
#include "../nodes/ui/image.h"
#include "../nodes/audio_source.h"
#include "splash.h"

#define SIMULANT_TEXT_512 "textures/simulant-text-512.png"
#define SIMULANT_TEXT_1024 "textures/simulant-text.png"

#define SIMULANT_LOGO_256 "textures/simulant-icon-256.png"
#define SIMULANT_LOGO_512 "textures/simulant-icon.png"

#define SIMULANT_SOUND "sounds/simulant.wav"

namespace smlt {
namespace scenes {

void Splash::load() {
    TextureFlags flags;
    flags.mipmap = smlt::MIPMAP_GENERATE_NONE;

    auto text_file = (window->width() < 1200) ? SIMULANT_TEXT_512 : SIMULANT_TEXT_1024;
    auto logo_file = (window->width() < 1200) ? SIMULANT_LOGO_256 : SIMULANT_LOGO_512;

    auto text_texture = assets->load_texture(text_file, flags);
    text_ = create_node<ui::Image>(text_texture);

    auto texture = assets->load_texture(logo_file, flags);
    image_ = create_node<ui::Image>(texture);

    sound_ = assets->load_sound(SIMULANT_SOUND, SoundFlags(), smlt::GARBAGE_COLLECT_NEVER);

    image_->set_anchor_point(0.5f, 0.0f);
    image_->set_opacity(0.0f);

    text_->set_anchor_point(0.5, 1.0);

    image_->transform->set_position_2d(window->coordinate_from_normalized(0.5f, 0.4f));
    text_->transform->set_position_2d(window->coordinate_from_normalized(0.5f, 0.4f));

    //Create an orthographic camera
    camera_ = create_node<Camera>();
    camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    source_ = camera_->create_child<AudioSource>();

    //Create an inactive pipeline
    pipeline_ = compositor->render(this, camera_);
    link_pipeline(pipeline_);
}

void Splash::unload() {
    //Clean up
    pipeline_->destroy();
}

void Splash::activate() {
    start_time_ = app->time_keeper->now_in_us();
    source_->play_sound(
        sound_,
        AUDIO_REPEAT_NONE,
        DISTANCE_MODEL_AMBIENT
    );
}

void Splash::deactivate() {
    //Deactivate the Splash pipeline

}

void Splash::on_update(float dt) {
    /* We can't use dt as we load the scene in the main thread, and
     * by the time we hit update dt will be large and we'll
     * we'll immediately start loading the next scene */
    _S_UNUSED(dt);

    const float FADE_SPEED = 2.0f;

    if(!this->is_active()) {
        return;
    }

    auto diff = app->time_keeper->now_in_us() - start_time_;
    float elapsed = float(diff) / 1000000.0f;

    image_->set_opacity(std::min(elapsed * FADE_SPEED, 1.0f));

    if(elapsed >= time_) {
        scenes->activate(target_);
    }
}

}
}
