#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"

using namespace kglt;
using namespace kglt::extra;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(kglt::WindowBase& window):
        kglt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        prepare_basic_scene(stage_id_, camera_id_);

        auto stage = window->stage(stage_id_);
        window->resource_locator->add_search_path("sample_data/q2");
        window->loader_for("sample_data/sample.bsp")->into(stage.get());

        stage->host_camera(camera_id_);
        stage->camera(camera_id_)->set_absolute_position(
            stage->get<kglt::Vec3>("player_spawn")
        );

        window->camera(camera_id_)->set_perspective_projection(
            45.0,
            float(window->width()) / float(window->height()),
            0.1,
            1000.0
        );

        stage->set_ambient_light(kglt::Colour(0.2, 0.2, 0.2, 1.0));
    }

    void do_activate() {

    }

private:
    StageID stage_id_;
    CameraID camera_id_;
};


class Q2Sample: public kglt::Application {
private:
    bool do_init() {
        register_screen("/", kglt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    Q2Sample app;
    return app.run();
}
