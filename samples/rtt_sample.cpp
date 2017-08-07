#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(WindowBase& window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        cube_stage_ = window->new_stage();        
        auto cube_cam = cube_stage_.fetch()->new_camera().fetch();

        rect_stage_ = window->new_stage();
        auto rect_cam = rect_stage_.fetch()->new_camera().fetch();

        smlt::TextureID tid = window->shared_assets->new_texture_from_file("sample_data/sample.tga");
        MeshID cube_mesh = window->shared_assets->new_mesh_as_cube(1.0);
        window->shared_assets->mesh(cube_mesh)->set_texture_on_material(0, tid);
        cube_ = window->stage(cube_stage_)->new_actor_with_mesh(cube_mesh);
        window->stage(cube_stage_)->actor(cube_)->move_to_absolute(0, 0, -4);

        MeshID rect_mesh = window->shared_assets->new_mesh_as_rectangle(2.0, 2.0);
        rect_ = window->stage(rect_stage_)->new_actor_with_mesh(rect_mesh);
        window->stage(rect_stage_)->actor(rect_)->move_to_absolute(0, 0, -4);

        TextureID rtt = window->shared_assets->new_texture(smlt::GARBAGE_COLLECT_NEVER);
        window->shared_assets->mesh(rect_mesh)->set_texture_on_material(0, rtt);

        window->render(cube_stage_, cube_cam->id()).to_texture(rtt);
        window->render(rect_stage_, rect_cam->id()).to_framebuffer(
            Viewport(VIEWPORT_TYPE_FULL, smlt::Colour::GREY)
        ).with_clear();
    }

    void fixed_update(float dt) {
        window->stage(cube_stage_)->actor(cube_)->rotate_y_by(Degrees(dt * 360));
        window->stage(rect_stage_)->actor(rect_)->rotate_y_by(Degrees(dt * 180));
    }

private:
    StageID cube_stage_, rect_stage_;
    ActorID cube_, rect_;
};

class Sample: public smlt::Application {
public:
    Sample(const AppConfig& config):
        Application(config) {
    }

private:
    bool do_init() {
        register_scene<GameScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "RTT Sample";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    Sample app(config);
    return app.run();
}
