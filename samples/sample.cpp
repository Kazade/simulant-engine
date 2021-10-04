
#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        stage_ = window->new_stage(smlt::PARTITIONER_FRUSTUM);
        camera_ = stage_->new_camera();
        auto pipeline = compositor->render(stage_, camera_);
        pipeline->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        link_pipeline(pipeline);

        pipeline->viewport->set_colour(smlt::Colour::BLUE);

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

        smlt::MeshPtr ms3d_mesh = stage_->assets->new_mesh_from_file(
            "sample_data/fellguard/fellguard_animated-ms3d.ms3d"
        );

        ms3d_mesh->add_animation("idle", 0, 100, /*FIXME*/7);

        auto actor2= stage_->new_actor_with_mesh(ms3d_mesh);
        actor2->move_to(-40.0f, 0.0f, -95.0f);
        actor2->rotate_global_y_by(smlt::Degrees(180));
        //actor2->animation_state->play_animation("idle");

        // Add a fly controller to the camera for user input
        camera_->new_behaviour<behaviours::Fly>(window);

        // Load a zombie sound and play it
        sound_ = stage_->assets->new_sound_from_file("sample_data/zombie.wav");
        actor->play_sound(sound_, AUDIO_REPEAT_FOREVER);
        actor->set_gain(1);
        actor->set_reference_distance(50);
    }

private:
    CameraPtr camera_;
    StagePtr stage_;

    SoundPtr sound_;
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
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Basic Sample";
    config.fullscreen = false;
    
#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    Sample app(config);
    return app.run();
}
