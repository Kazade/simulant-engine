#include "simulant/simulant.h"
#include "simulant/shortcuts.h"


class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::Core* core):
        smlt::Scene<MainScene>(core) {}

    void load() {
        // Create two viewports for the left and right hand side of the screen, set different clear colours
        smlt::Viewport first(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Colour::RED);
        smlt::Viewport second(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, smlt::Colour::GREEN);

        auto stage = core->new_stage();

        auto cube = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        cube->new_submesh_as_cube("cube", stage->assets->new_material(), 1.0);
        smlt::ActorPtr actor = stage->new_actor_with_mesh(cube);

        actor->move_to(0, 0, -5);

        // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
        compositor->render(
            stage, stage->new_camera_for_viewport(first)
        )->set_viewport(
            first
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);

        compositor->render(
            stage, stage->new_camera_for_viewport(second)
        )->set_viewport(
            second
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    }
};

class ViewportSample : public smlt::Application {
public:
    ViewportSample(const smlt::AppConfig& config):
        smlt::Application(config) {}

    bool init() {
        scenes->register_scene<MainScene>("main");
        return true;
    }
};


int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Viewport Sample";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    ViewportSample app(config);
    return app.run();
}
