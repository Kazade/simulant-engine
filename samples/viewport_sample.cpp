#include "kglt/kglt.h"
#include "kglt/shortcuts.h"


class MainScreen : public kglt::Screen<MainScreen> {
public:
    MainScreen(kglt::WindowBase& window):
        kglt::Screen<MainScreen>(window, "main_screen") {}

    void do_load() {
        // Create two viewports for the left and right hand side of the screen, set different clear colours
        kglt::Viewport first(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, kglt::Colour::RED);
        kglt::Viewport second(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, kglt::Colour::GREEN);

        kglt::StageID sid = window->new_stage();
        auto stage = window->stage(sid);

        kglt::MeshID cube = stage->resources->new_mesh_as_cube(1.0);
        kglt::ActorID aid = stage->new_actor_with_mesh(cube);

        stage->actor(aid)->move_to(0, 0, -5);

        // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
        window->render(sid, window->new_camera_for_viewport(first)).to_framebuffer(first).with_clear();
        window->render(sid, window->new_camera_for_viewport(second)).to_framebuffer(second).with_clear();
    }
};

class ViewportSample : public kglt::Application {
public:
    bool do_init() {
        register_screen("/", kglt::screen_factory<MainScreen>());
        return true;
    }
};


int main(int argc, char* argv[]) {
    ViewportSample app;
    return app.run();
}
