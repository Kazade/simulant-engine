#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(smlt::Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        pipeline_ = prepare_basic_scene(stage_, camera_);
        pipeline_->set_clear_flags(BUFFER_CLEAR_ALL);
        pipeline_->viewport->set_colour(smlt::Colour::GREY);
        pipeline_->deactivate();

        window->resource_locator->add_search_path("sample_data/quake2/textures");

        auto mesh = stage_->assets->new_mesh_from_file("sample_data/quake2/maps/aggression.bsp").fetch();
        auto actor_id = stage_->new_geom_with_mesh(mesh->id());
        /*
        stage->camera(camera_id_)->move_to_absolute(
            mesh->data->get<smlt::Vec3>("player_spawn")
        );*/

        // Add a fly controller to the camera for user input
        camera_->new_behaviour<behaviours::Fly>(window);

        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            1.0,
            10000.0
        );

        stage_->set_ambient_light(smlt::Colour(0.8, 0.8, 0.8, 1.0));
    }

    void activate() {
        pipeline_->activate();
    }

private:
    StagePtr stage_;
    CameraPtr camera_;
    PipelinePtr pipeline_;
};


class Q2Sample: public smlt::Application {
public:
    Q2Sample(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        scenes->load_in_background("main", true); //Do loading in a background thread, but show immediately when done
        scenes->activate("_loading"); // Show the loading screen in the meantime
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
