#include "simulant/simulant.h"
#include "simulant/shortcuts.h"


class MainScene : public smlt::Scene {
public:
    MainScene(smlt::Window* window):
        smlt::Scene(window) {}

    void on_load() override {
        // Create two viewports for the left and right hand side of the screen, set different clear colors
        smlt::Viewport first(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT,
                             smlt::Color::red());
        smlt::Viewport second(smlt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT,
                              smlt::Color::green());

        auto cube = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        cube->create_submesh_as_cube("cube", assets->create_material(), 1.0);
        smlt::ActorPtr actor = create_child<smlt::Actor>(cube);

        actor->transform->set_position(smlt::Vec3(0, 0, -5));

        auto camera1 = create_child<smlt::Camera>();
        auto camera2 = create_child<smlt::Camera>();

        // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
        compositor->create_layer(
            this, camera1
        )->set_viewport(
            first
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);

        compositor->create_layer(
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
