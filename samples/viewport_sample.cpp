#include "simulant/simulant.h"
#include "simulant/shortcuts.h"


class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::WindowBase& window):
        smlt::Scene<MainScene>(window, "main_screen") {}

    void do_load() {
        // Create two viewports for the left and right hand side of the screen, set different clear colours
        smlt::Viewport first(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Colour::RED);
        smlt::Viewport second(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, smlt::Colour::GREEN);

        smlt::StageID sid = window->new_stage();
        auto stage = window->stage(sid);

        smlt::MeshID cube = stage->assets->new_mesh_as_cube(1.0);
        smlt::ActorID aid = stage->new_actor_with_mesh(cube);

        stage->actor(aid)->move_to(0, 0, -5);

        // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
        window->render(sid, window->new_camera_for_viewport(first)).to_framebuffer(first).with_clear();
        window->render(sid, window->new_camera_for_viewport(second)).to_framebuffer(second).with_clear();
    }
};

class ViewportSample : public smlt::Application {
public:
    bool do_init() {
        register_scene<MainScene>("main");
        return true;
    }
};


int main(int argc, char* argv[]) {
    ViewportSample app;
    return app.run();
}
