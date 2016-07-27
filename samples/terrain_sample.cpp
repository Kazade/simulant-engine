#include <kazbase/random.h>

#include "kglt/kglt.h"

using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(kglt::WindowBase& window):
        kglt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        pipeline_id_ = prepare_basic_scene(stage_id_, camera_id_, kglt::PARTITIONER_NULL);
        window->disable_pipeline(pipeline_id_);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);
        window->camera(camera_id_)->set_perspective_projection(45.0, float(window->width()) / float(window->height()), 10.0, 10000.0);

        window->pipeline(pipeline_id_)->viewport->set_colour(kglt::Colour::SKY_BLUE);

        auto cam = stage->camera(camera_id_);
        cam->move_to(0, 0, 500);
        cam->look_at(0, 0, 0);

        terrain_mesh_id_ = window->new_mesh_from_heightmap("sample_data/terrain.png", 2.5);
        terrain_actor_id_ = stage->new_actor_with_mesh(terrain_mesh_id_);
    }

    void do_activate() {
        window->enable_pipeline(pipeline_id_);
    }

    void do_step(double dt) override {
        auto stage = window->stage(stage_id_);
        stage->actor(terrain_actor_id_)->rotate_global_y(kglt::Degrees(dt * 5.0));
    }

private:
    PipelineID pipeline_id_;
    StageID stage_id_;
    CameraID camera_id_;

    MeshID terrain_mesh_id_;
    ActorID terrain_actor_id_;
};


class TerrainDemo: public kglt::Application {
public:
    TerrainDemo():
        kglt::Application("Terrain Demo") {}

private:
    bool do_init() {
        register_screen("/", kglt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    TerrainDemo app;
    return app.run();
}
