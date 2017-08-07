#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;
using namespace smlt::extra;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(smlt::WindowBase& window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        pid_ = prepare_basic_scene(stage_id_, camera_id_);
        window->pipeline(pid_)->set_clear_flags(BUFFER_CLEAR_ALL);
        window->pipeline(pid_)->viewport->set_colour(smlt::Colour::GREY);
        window->disable_pipeline(pid_);

        auto stage = window->stage(stage_id_);
        window->resource_locator->add_search_path("sample_data/q2");

        auto mesh = stage->assets->new_mesh_from_file("sample_data/sample.bsp").fetch();
        auto actor_id = stage->new_actor_with_mesh(mesh->id());
        /*
        stage->camera(camera_id_)->move_to_absolute(
            mesh->data->get<smlt::Vec3>("player_spawn")
        );*/

        // Add a fly controller to the camera for user input
        stage->camera(camera_id_)->new_controller<controllers::Fly>(window);

        camera_id_.fetch()->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );

        stage->set_ambient_light(smlt::Colour(0.8, 0.8, 0.8, 1.0));
    }

    void activate() {
        window->enable_pipeline(pid_);
    }

private:
    StageID stage_id_;
    CameraID camera_id_;
    PipelineID pid_;
};


class Q2Sample: public smlt::Application {
public:
    Q2Sample(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        register_scene<GameScene>("main");
        load_scene_in_background("main", true); //Do loading in a background thread, but show immediately when done
        activate_scene("_loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Quake 2 Mesh Loader";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    Q2Sample app(config);
    return app.run();
}
