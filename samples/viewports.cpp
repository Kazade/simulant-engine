#include "simulant/simulant.h"
#include "simulant/shortcuts.h"


class MainScene : public smlt::Scene {
public:
    MainScene(smlt::Window* window):
        smlt::Scene(window) {}

    void load() {
        // Create two viewports for the left and right hand side of the screen, set different clear colors
        smlt::Viewport first(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, smlt::Color::RED);
        smlt::Viewport second(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, smlt::Color::GREEN);

        auto cube = assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        cube->new_submesh_as_cube("cube", assets->new_material(), 1.0);
        smlt::ActorPtr actor = create_node<smlt::Actor>(cube);

        actor->transform->set_position(smlt::Vec3(0, 0, -5));

        auto camera1 = create_node<smlt::Camera>();
        auto camera2 = create_node<smlt::Camera>();

        // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
        compositor->render(
            this, camera1
        )->set_viewport(
            first
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);

        compositor->render(
            this, camera2
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
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Viewport Sample";
    config.fullscreen = false;
    
#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    ViewportSample app(config);
    return app.run();
}
