#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        cube_stage_ = new_stage();
        auto cube_cam = cube_stage_->new_camera();

        rect_stage_ = new_stage();
        auto rect_cam = rect_stage_->new_camera();

        auto tid = app->shared_assets->new_texture_from_file("assets/samples/sample.tga");
        auto cube_mesh = app->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        cube_mesh->new_submesh_as_cube("cube", app->shared_assets->new_material(), 1.0);

        auto mat = cube_mesh->first_submesh()->material();
        mat->set_diffuse_map(tid);

        cube_ = cube_stage_->new_actor_with_mesh(cube_mesh);
        cube_->move_to_absolute(0, 0, -4);

        auto rect_mesh = app->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        rect_mesh->new_submesh_as_rectangle("rect", app->shared_assets->new_material(), 2.0, 2.0);
        rect_ = rect_stage_->new_actor_with_mesh(rect_mesh);
        rect_->move_to_absolute(0, 0, -4);

        auto rtt = app->shared_assets->new_texture(8, 8, TEXTURE_FORMAT_RGBA_4UB_8888, smlt::GARBAGE_COLLECT_NEVER);
        mat = rect_mesh->first_submesh()->material();
        mat->set_diffuse_map(rtt);

        compositor->render(
            cube_stage_,
            cube_cam
        )->set_target(rtt);

        compositor->render(
            rect_stage_, rect_cam
        )->set_viewport(
            Viewport(VIEWPORT_TYPE_FULL, smlt::Colour::GREY)
        )->set_clear_flags(BUFFER_CLEAR_ALL);
    }

    void fixed_update(float dt) {
        cube_->rotate_y_by(Degrees(dt * 360));
        rect_->rotate_y_by(Degrees(dt * 180));
    }

private:
    StagePtr cube_stage_, rect_stage_;
    ActorPtr cube_ = nullptr;
    ActorPtr rect_ = nullptr;
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
    config.title = "RTT Sample";
    config.fullscreen = false;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    Sample app(config);
    return app.run(argc, argv);
}
