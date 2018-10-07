

#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        stage_ = window->new_stage_from_file("sample_data/model.dae");
        camera_ = stage_->new_camera();
        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );
        // Add a fly controller to the camera for user input
        camera_->new_behaviour<behaviours::Fly>(window);
    }

private:
    PipelinePtr pipeline_;
    CameraPtr camera_;
    StagePtr stage_;
};

class Sample: public smlt::Application {
public:
    Sample(const AppConfig& config):
        Application(config) {
    }

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Basic Sample";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    Sample app(config);
    return app.run();
}
