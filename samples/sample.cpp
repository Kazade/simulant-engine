#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(WindowBase& window):
        smlt::Scene<GameScene>(window, "game_screen") {}

    void do_load() {
        auto pipeline_id = prepare_basic_scene(stage_id_, camera_id_, smlt::PARTITIONER_NULL);

        window->pipeline(pipeline_id)->viewport->set_colour(smlt::Colour::SKY_BLUE);

        auto stage = window->stage(stage_id_);

        window->camera(camera_id_)->set_perspective_projection(
            45.0,
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );

        stage->set_ambient_light(smlt::Colour::WHITE);

        // Load an animated MD2 mesh
        smlt::MeshID mesh_id = stage->assets->new_mesh_from_file("sample_data/ogro.md2");

        auto actor = stage->new_actor_with_mesh(mesh_id).fetch(); // Create an instance of it
        actor->move_to(0.0f, 0.0f, -80.0f);
        actor->rotate_global_y_by(smlt::Degrees(180));

        auto actor3 = stage->new_actor_with_mesh(mesh_id).fetch();
        actor3->move_to(-40.0f, 0.0f, -95.0f);
        actor3->rotate_global_y_by(smlt::Degrees(180));
        actor3->animation_state->play_animation("idle_2");

        auto scaling_matrix = smlt::Mat4::as_scaling(10.0);

        auto tank = stage->assets->new_mesh_from_file("sample_data/tank.obj").fetch();
        tank->transform_vertices(scaling_matrix);

        auto tank_actor = stage->new_actor_with_mesh(tank->id()).fetch();
        tank_actor->move_to(40, 0, -110);
    }

private:
    CameraID camera_id_;
    StageID stage_id_;
};

class Sample: public smlt::Application {
public:
    Sample():
        Application("Simulant Sample") {
    }

private:
    bool do_init() {
        register_scene("/", scene_factory<GameScene>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    Sample app;
    return app.run();
}
