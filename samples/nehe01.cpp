
#include <kglt/kglt.h>

class MainScreen : public kglt::Screen<MainScreen> {
public:
    MainScreen(kglt::WindowBase& window):
        kglt::Screen<MainScreen>(window, "NeHe 01") {}

    void do_load() {
        prepare_basic_scene(stage_, camera_);

        kglt::StagePtr stage = stage_.fetch();
        kglt::MeshPtr square = stage->assets->new_mesh_as_rectangle(1.0, 1.0).fetch();

        auto actor = stage->new_actor_with_mesh(square->id()).fetch();
        actor->move_to(0, 0, -5);
        actor->scale_by(2.0);
    }

private:
    kglt::StageID stage_;
    kglt::CameraID camera_;
};

class App : public kglt::Application {
public:
    bool do_init() {
        register_screen("/", kglt::screen_factory<MainScreen>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    App app;
    return app.run();
}
