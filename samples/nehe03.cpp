
#include <simulant/simulant.h>

class MainScene : public smlt::Scene {
public:
    MainScene(smlt::Window* window):
        smlt::Scene(window) {}

    void load() {
        camera_ = create_node<smlt::Camera>();
        auto pipeline = compositor->render(this, camera_);
        link_pipeline(pipeline);

        smlt::MeshPtr square = assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        square->new_submesh_as_rectangle("rect", assets->new_material(), 1.0, 1.0);

        square->vertex_data->move_to(0);
        square->vertex_data->diffuse(smlt::Colour::RED);
        square->vertex_data->move_next();

        square->vertex_data->diffuse(smlt::Colour::GREEN);
        square->vertex_data->move_next();

        square->vertex_data->diffuse(smlt::Colour::BLUE);
        square->vertex_data->move_next();

        square->vertex_data->diffuse(smlt::Colour::YELLOW);
        square->vertex_data->move_to_end();
        square->vertex_data->done();

        auto actor = create_node<smlt::Actor>(square);
        actor->move_to(0, 0, -5);
        actor->scale_by(2.0);

        S_DEBUG("Scene loaded");
    }

private:
    smlt::StagePtr stage_;
    smlt::CameraPtr camera_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

        window->set_logging_level(smlt::LOG_LEVEL_DEBUG);
    }

    bool init() {
        scenes->register_scene<MainScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "NeHe 01";

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    App app(config);
    return app.run();
}
