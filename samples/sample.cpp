
#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        stage_ = new_stage(smlt::PARTITIONER_FRUSTUM);
        camera_ = stage_->new_camera();
        auto pipeline = compositor->render(stage_, camera_);
        pipeline->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        link_pipeline(pipeline);

        pipeline->viewport->set_colour(smlt::Colour::BLUE);

        camera_->set_perspective_projection(
            Degrees(45.0),
            (double) (float(window->width()) / float(window->height())),
            0.1,
            1000.0
        );

        stage_->new_light_as_directional();
        stage_->set_ambient_light(smlt::Colour(0.3, 0.3, 0.3, 1.0));

        auto mesh = stage_->assets->new_mesh_from_file("sample_data/autumn_house/autumn_house.obj");

        actor_ = stage_->new_actor_with_mesh(mesh); // Create an instance of it
        actor_->move_to(0.0f, -0.05f, -1.0f);
        actor_->rotate_global_y_by(smlt::Degrees(-90.0f));

        // Add a fly controller to the camera for user input
        auto b = camera_->new_behaviour<behaviours::Fly>(window);
        b->set_speed(1.0f);
    }

    void update(float dt) override {
        actor_->rotate_global_y_by(smlt::Degrees(1.0f * dt));
    }

private:
    CameraPtr camera_;
    StagePtr stage_;

    SoundPtr sound_;
    ActorPtr actor_;
};

class Sample: public smlt::Application {
public:
    Sample(const AppConfig& config):
        Application(config) {
    }

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        // scenes->register_scene<smlt::scenes::Splash>("main", "ingame");
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
