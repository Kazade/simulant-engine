#include "kglt/kglt.h"

using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        kglt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        auto pipeline_id = prepare_basic_scene(stage_id_, camera_id_);

        window->pipeline(pipeline_id)->viewport->set_colour(kglt::Colour::SKY_BLUE);

        auto stage = window->stage(stage_id_);

        window->camera(camera_id_)->set_perspective_projection(
            45.0,
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );

        stage->set_ambient_light(kglt::Colour::WHITE);

        // Load an animated MD2 mesh
        kglt::MeshID mesh_id = stage->assets->new_mesh_from_file("sample_data/ogro.md2");

        // Create an instance of it
        auto actor = stage->new_actor_with_mesh(mesh_id).fetch();
        actor->move_to(0.0f, 0.0f, -80.0f);
        actor->rotate_global_y(kglt::Degrees(180));
    }

    void do_step(double dt) {

    }

private:
    CameraID camera_id_;
    StageID stage_id_;
};

class Sample: public kglt::Application {
public:
    Sample():
        Application("KGLT Combined Sample") {
    }

private:
    bool do_init() {
        register_screen("/", screen_factory<GameScreen>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    Sample app;
    return app.run();
}
