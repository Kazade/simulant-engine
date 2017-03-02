
#include <simulant/simulant.h>

class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::WindowBase& window):
        smlt::Scene<MainScene>(window) {}

    void do_load() {
        prepare_basic_scene(stage_, camera_);

        smlt::StagePtr stage = stage_.fetch();
        smlt::MeshPtr square = stage->assets->new_mesh_as_rectangle(1.0, 1.0).fetch();

        auto actor = stage->new_actor_with_mesh(square->id()).fetch();
        actor->move_to(0, 0, -5);
        actor->scale_by(2.0);
    }

private:
    smlt::StageID stage_;
    smlt::CameraID camera_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {}

    bool do_init() {
        register_scene<MainScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "NeHe 01";
    App app(config);
    return app.run();
}
