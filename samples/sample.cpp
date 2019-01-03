#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        auto pipeline = prepare_basic_scene(stage_, camera_, smlt::PARTITIONER_NULL);
        pipeline->viewport->set_colour(smlt::Colour::SKY_BLUE);

        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );

        stage_->new_light_as_directional(smlt::Vec3(1, -1, 0));
        stage_->new_light_as_directional(smlt::Vec3(-1, 0, 0), smlt::Colour::RED);
        stage_->set_ambient_light(smlt::Colour(0.3, 0.3, 0.3, 1.0));

        // Load an animated MD2 mesh
        smlt::MeshID mesh_id = stage_->assets->new_mesh_from_file("sample_data/ogro.md2");

        auto actor = stage_->new_actor_with_mesh(mesh_id); // Create an instance of it
        actor->move_to(0.0f, 0.0f, -80.0f);
        actor->rotate_global_y_by(smlt::Degrees(180));

        auto actor3 = stage_->new_actor_with_mesh(mesh_id);
        actor3->move_to(-40.0f, 0.0f, -95.0f);
        actor3->rotate_global_y_by(smlt::Degrees(180));
        actor3->animation_state->play_animation("idle_2");

        // Add a fly controller to the camera for user input
        camera_->new_behaviour<behaviours::Fly>(window);

        // Make the camera the audio listener
        window->set_audio_listener(camera_);

        sounds_[0] = window->shared_assets->new_sound_from_file(
            "sample_data/sounds/monster-1.wav"
        );

        sounds_[1] = window->shared_assets->new_sound_from_file(
            "sample_data/sounds/monster-2.wav"
        );

        sounds_[2] = window->shared_assets->new_sound_from_file(
            "sample_data/sounds/monster-3.wav"
        );

        // Play a sound every 2.5 seconds from actor
        window->idle->add_timeout(2.5, [=]() -> bool {
            static int sound = 0;
            actor->play_sound(sounds_[sound]);

            sound++;
            if(sound == 3) sound = 0;
        });

        // Play a sound every 3.0 seconds from actor3
        window->idle->add_timeout(3.0, [=]() -> bool {
            static int sound = 0;
            actor3->play_sound(sounds_[sound]);

            sound++;
            if(sound == 3) sound = 0;
        });
    }

private:
    CameraPtr camera_;
    StagePtr stage_;

    SoundID sounds_[3];
};

class Sample: public smlt::Application {
public:
    Sample(const AppConfig& config):
        Application(config) {
    }

private:
    bool init() {
        scenes->register_scene<GameScene>("ingame");
        scenes->register_scene<smlt::scenes::Splash>("main", "ingame");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Basic Sample";
    config.fullscreen = false;
    config.width = 640;
    config.height = 480;

    Sample app(config);
    return app.run();
}
